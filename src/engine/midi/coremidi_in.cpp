#include "engine/midi/coremidi_in.h"

// The MIDIPacketList / MIDIPacketNext read-callback API is deprecated in favour
// of MIDIEventList, but remains fully functional and is simpler for a single
// input port. Silence the deprecation noise locally (matches coremidi_out.cpp).
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace ai_arranger::midi {

namespace {

std::string sourceDisplayName(MIDIEndpointRef ep) {
    if (ep == 0) return {};
    CFStringRef cf = nullptr;
    if (MIDIObjectGetStringProperty(ep, kMIDIPropertyDisplayName, &cf) != noErr || !cf) {
        return {};
    }
    char buf[256] = {0};
    CFStringGetCString(cf, buf, sizeof(buf), kCFStringEncodingUTF8);
    CFRelease(cf);
    return std::string(buf);
}

} // namespace

CoreMidiIn::~CoreMidiIn() {
    shutdown();
}

bool CoreMidiIn::initialize(const char* clientName) noexcept {
    if (initialized_.load(std::memory_order_acquire)) return true;

    CFStringRef cfClient = CFStringCreateWithCString(
        kCFAllocatorDefault, clientName ? clientName : "AI Arranger", kCFStringEncodingUTF8);
    OSStatus st = MIDIClientCreate(cfClient, &CoreMidiIn::notifyProc, this, &client_);
    if (cfClient) CFRelease(cfClient);
    if (st != noErr) return false;   // hard CoreMIDI failure

    CFStringRef cfPort = CFStringCreateWithCString(
        kCFAllocatorDefault, "in", kCFStringEncodingUTF8);
    st = MIDIInputPortCreate(client_, cfPort, &CoreMidiIn::readProc, this, &in_port_);
    if (cfPort) CFRelease(cfPort);
    if (st != noErr) {
        MIDIClientDispose(client_);
        client_ = 0;
        return false;
    }

    initialized_.store(true, std::memory_order_release);
    return true;   // zero sources is a valid headless state
}

void CoreMidiIn::shutdown() noexcept {
    if (!initialized_.load(std::memory_order_acquire)) return;
    // Disconnect any live source before tearing the port down.
    MIDIEndpointRef ep = source_.exchange(0, std::memory_order_acq_rel);
    if (ep != 0 && in_port_ != 0) MIDIPortDisconnectSource(in_port_, ep);
    if (in_port_) { MIDIPortDispose(in_port_); in_port_ = 0; }
    if (client_)  { MIDIClientDispose(client_); client_ = 0; }
    selected_index_.store(-1, std::memory_order_release);
    initialized_.store(false, std::memory_order_release);
}

bool CoreMidiIn::isInitialized() const noexcept {
    return initialized_.load(std::memory_order_acquire);
}

int CoreMidiIn::sourceCount() const noexcept {
    return static_cast<int>(MIDIGetNumberOfSources());
}

std::vector<MidiSourceInfo> CoreMidiIn::enumerateSources() const noexcept {
    std::vector<MidiSourceInfo> out;
    ItemCount n = MIDIGetNumberOfSources();
    out.reserve(n);
    for (ItemCount i = 0; i < n; ++i) {
        MIDIEndpointRef ep = MIDIGetSource(i);
        out.push_back({static_cast<int>(i), sourceDisplayName(ep)});
    }
    return out;
}

bool CoreMidiIn::selectSource(int index) noexcept {
    if (!initialized_.load(std::memory_order_acquire)) return false;

    // Disconnect the previous source first.
    MIDIEndpointRef prev = source_.exchange(0, std::memory_order_acq_rel);
    if (prev != 0) MIDIPortDisconnectSource(in_port_, prev);

    if (index < 0) {   // explicit "none"
        selected_index_.store(-1, std::memory_order_release);
        selected_name_.clear();
        return true;
    }
    if (index >= static_cast<int>(MIDIGetNumberOfSources())) return false;

    MIDIEndpointRef ep = MIDIGetSource(static_cast<ItemCount>(index));
    if (ep == 0) return false;
    if (MIDIPortConnectSource(in_port_, ep, nullptr) != noErr) return false;

    source_.store(ep, std::memory_order_release);
    selected_index_.store(index, std::memory_order_release);
    selected_name_ = sourceDisplayName(ep);
    return true;
}

int CoreMidiIn::selectedSource() const noexcept {
    return selected_index_.load(std::memory_order_acquire);
}

bool CoreMidiIn::hasLiveSource() const noexcept {
    return source_.load(std::memory_order_acquire) != 0;
}

// ── Read callback (CoreMIDI read thread) ─────────────────────────────
void CoreMidiIn::readProc(const MIDIPacketList* pktList, void* readRefCon, void* /*srcRefCon*/) {
    auto* self = static_cast<CoreMidiIn*>(readRefCon);
    if (!self || !pktList) return;

    const MIDIPacket* pkt = &pktList->packet[0];
    for (UInt32 i = 0; i < pktList->numPackets; ++i) {
        MidiInputMessage msgs[64];
        const size_t n = parseMidiInput(pkt->data, pkt->length, msgs, 64);
        if (self->sink_) {
            for (size_t j = 0; j < n; ++j) self->sink_(msgs[j]);
        }
        self->received_.fetch_add(n, std::memory_order_relaxed);
        pkt = MIDIPacketNext(pkt);
    }
}

void CoreMidiIn::notifyProc(const MIDINotification* /*msg*/, void* /*refCon*/) {
    // Hot-plug notifications arrive here when a CoreMIDI run loop is present.
    // Source re-resolution by name is a follow-up (mirrors CoreMidiOut); the
    // port keeps working for the currently-connected source regardless.
}

} // namespace ai_arranger::midi

#pragma clang diagnostic pop
