#ifndef AI_ARRANGER_COREAUDIO_DRIVER_H
#define AI_ARRANGER_COREAUDIO_DRIVER_H

#include "engine/realtime/clock.h"
#include "engine/midi/scheduler.h"
#include <AudioToolbox/AudioToolbox.h>
#include <functional>
#include <atomic>

namespace ai_arranger::realtime {

/**
 * CoreAudio realtime driver.
 *
 * Opens a default output AudioUnit and drives the RealtimeClock
 * from the render callback's sample count.
 *
 * Midi events are dispatched from the render callback via the
 * MIDI scheduler (lock-free queue).
 *
 * Architecture rules:
 * - No malloc inside render callback
 * - No mutex inside render callback
 * - No file IO inside render callback
 * - No ObjC message dispatch inside render callback
 */

class CoreAudioDriver {
public:
    CoreAudioDriver();
    ~CoreAudioDriver();

    // No copy
    CoreAudioDriver(const CoreAudioDriver&) = delete;
    CoreAudioDriver& operator=(const CoreAudioDriver&) = delete;

    // ── Lifecycle ──────────────────────────────────────────────────
    bool init(uint32_t sampleRate = 48000) noexcept;
    bool start() noexcept;
    void stop() noexcept;

    // ── Accessors ───────────────────────────────────────────────────
    RealtimeClock&       clock() noexcept       { return clock_; }
    midi::MidiScheduler& scheduler() noexcept   { return scheduler_; }
    bool isRunning() const noexcept             { return running_.load(std::memory_order_acquire); }
    uint32_t bufferSizeFrames() const noexcept  { return buffer_size_; }
    AudioUnit audioUnit() const noexcept        { return audio_unit_; }

private:
    // CoreAudio render callback (REALTIME SAFE)
    static OSStatus renderCallback(void* inRefCon,
                                    AudioUnitRenderActionFlags* ioActionFlags,
                                    const AudioTimeStamp* inTimeStamp,
                                    UInt32 inBusNumber,
                                    UInt32 inNumFrames,
                                    AudioBufferList* ioData);

    OSStatus onRender(AudioUnitRenderActionFlags* ioActionFlags,
                      const AudioTimeStamp* inTimeStamp,
                      UInt32 inBusNumber,
                      UInt32 inNumFrames,
                      AudioBufferList* ioData);

    RealtimeClock      clock_;
    midi::MidiScheduler scheduler_;
    AudioUnit          audio_unit_{nullptr};
    AudioComponent     component_{nullptr};
    uint32_t           buffer_size_{256};
    std::atomic<bool>  running_{false};
    std::atomic<int64_t> total_samples_{0};
};

} // namespace ai_arranger::realtime
#endif // AI_ARRANGER_COREAUDIO_DRIVER_H
