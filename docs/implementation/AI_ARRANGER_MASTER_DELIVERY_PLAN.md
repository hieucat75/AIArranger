# AI ARRANGER / STYLE MAKER — MASTER DELIVERY PLAN

> **Project:** Live Arranger Platform (iOS/iPadOS)  
> **Version:** 2.0 — Revised Sound Strategy & User Sound Library Policy  
> **Date:** 2026-06-13  
> **Status:** ⚡ APPROVED FOR EXECUTION — REVISED PER ADR-013  
> **Source Documents:** 19 spec/plan files (v0.9) + ADR-013

---

## TABLE OF CONTENTS

1. [A. Executive Deployment Doctrine](#a-executive-deployment-doctrine)
2. [B. Workstream Breakdown (12 Workstreams)](#b-workstream-breakdown-12-workstreams)
3. [C. Gate-Based Delivery Plan (Gate 0 → Gate 10)](#c-gate-based-delivery-plan-gate-0--gate-10)
4. [D. RACI Matrix](#d-raci-matrix)
5. [Appendices](#appendices)

---

# A. EXECUTIVE DEPLOYMENT DOCTRINE

## 10 Immutable Deployment Principles

These are **not guidelines**. These are rules. Violation = unacceptable compromise. If you cannot ship under them, stop and re-scope.

---

### DOCTRINE 01 — Tower of Time (Timing First, Everything Else Second)

**The single non-negotiable:** MIDI event timing accuracy is the product's soul. A style that plays wrong notes at the right time is fixable. A style that plays right notes at the wrong time is **useless on stage**.

| Priority | What | Why | Penalty for failure |
|---|---|---|---|
| **#1** | Timing accuracy | Section switch jitter is the #1 reason musicians abandon arranger apps | **Kill signal.** If Gate 1 spike fails, re-evaluate the entire project. |
| **#2** | MIDI stability & correctness | Notes must land on correct ticks; no orphan NoteOns, no stuck voices; stable external MIDI connectivity | **P0 bug.** Blocks all subsequent gates. |
| **#3** | Arranger logic | NTT/NTR/retrigger semantics must be musically correct | **P1.** Tolerate minor differences; refine via A/B testing. |
| **#4** | Live UX | Glanceable, fat-finger-safe, performance mode, stage-ready reliability | **P1.** Must pass musician validation. |
| **#5** | Import compatibility | SFF1 ≥ 95% success, SFF2 ≥ 75% success | **P1.** Graceful degradation with clear limitations. |
| **#6** | External playback ecosystem | USB/BLE/Network MIDI device support, AUv3 hosting readiness | **P1.** Beta exit criterion for external MIDI. |
| **#7** | User-loaded sound libraries | SF2/SFZ import, articulation detection, UI for sound selection | **P2.** Phase 2 for full implementation. |
| **#8** | Internal articulation engine | GM-fallback quality, velocity mapping improvements | **P2.** Post-MVP refinement. |
| **#9** | Commercial layer & proprietary ecosystem | Pricing, App Store compliance, premium sound marketplace | **P2** (pricing) / **P3** (marketplace). |
| **#10** | AI/Marketplace | Deferred to Phase 2/3 | **P3.** Do not touch in MVP. |

**Implementation mandate:** The audio clock is driven by the CoreAudio render callback sample position. No OS timers, no `DispatchSourceTimer`, no `NSTimer`. Every tick is derived from `currentPosition += ticksPerSample * nFrames`.

---

### DOCTRINE 02 — Sound Strategy Doctrine (Revised per ADR-013)

**Core principle:** The app is an **arranger platform** — its value is musical workflow, not sound generation. Sound sources are user-provided, not app-bundled.

**Explicitly in scope for Phase 1:**
- Arranger engine (section state machine, chord transformation)
- MIDI scheduler (sample-accurate event scheduling)
- Style playback with NTT/NTR/retrigger
- Chord engine (fingered mode, chord detection)
- Import pipeline (SFF1/SFF2 → UASF)
- Live UX (section pads, transport, mixer)
- External MIDI keyboard input & external tone generator output
- AUv3 instrument support (architecture readiness)
- User-loaded sample library infrastructure (SF2/SFZ paths)

**Explicitly NOT in scope for Phase 1 (sound generation):**
- Full internal Yamaha-like sound engine
- Custom synthesis models
- Hardware ROM emulation
- Bundled premium voice banks

**Sound source hierarchy (priority order):**
1. External MIDI device (USB/BLE/Network — highest fidelity, professional use)
2. AUv3 instrument (user-loaded plugin — Phase 2 full host)
3. User-loaded SF2/SFZ library (user-provided samples)
4. Bundled GM preview bank (degraded preview only — Phase 1)

**Internal sound scope limitation:**
The bundled GM bank (sfizz + GeneralUser GS) is strictly for **degraded preview** and **practice** — single layer, no velocity switching. It MUST NOT be marketed or documented as a "sound engine."

**Routing architecture:**
All events route identically regardless of output target (MIDI device, internal, AUv3, virtual). Routing is configuration, not a code fork. The engine calls `OutputSink::sendEvent(MIDIEvent)` — the sink is a pluggable abstraction.

**Why this matters for musicians:**
- Professional musicians already own sound hardware — the app leverages their investment
- No forced upgrade path or bundled sound quality disappointment
- Clear separation: arranger app ≠ sound module
- Future AUv3 and premium library support preserves upgrade path

**Legal rationale:**
- No proprietary sample bundling = no DMCA/Copyright liability
- User-provided content = user responsibility
- Vendor-neutral language = no trademark infringement risk

---

### DOCTRINE 03 — JUCE Is the Audio Backbone, Not the UI

**Architecture boundary:** JUCE = C++ audio/MIDI/DSP engine layer. SwiftUI = all user interface.

| Layer | Tech | Allowed to import |
|---|---|---|
| UI | SwiftUI, UIKit | Bridge headers only |
| Bridge | Swift/C++ Interop (Xcode 15+) | Swift ↔ C++ type marshalling |
| Core Engine | C++17 | Standard library, JUCE modules |
| Platform Adapter | C++ + Objective-C++ | CoreMIDI, CoreAudio, AVAudioEngine |

**No Swift in the audio callback.** No ObjC ARC in the audio callback. No exceptions in the audio callback. C++ with `-fno-exceptions` for audio thread code.

Swift/C++ interop API surface must stay thin: commands go **into** the engine via lock-free queue, state comes **out** via atomic snapshot. No direct function calls from Swift into audio thread objects.

---

### DOCTRINE 04 — The Audio Thread Is Sacred Ground

This thread is the CoreAudio render callback. It runs at real-time priority. Any violation causes audible glitches.

**ABSOLUTELY FORBIDDEN in audio callback:**

| Action | Consequence | Alternative |
|---|---|---|
| `malloc()` / `new` / `delete` | Heap contention → glitch | Pre-allocate all buffers at style load. Use pool allocators. |
| `std::mutex` / `pthread_mutex` | Priority inversion → deadlock possible | Lock-free SPSC queues for all cross-thread communication. |
| File I/O of any kind | Blocking → callback overrun → audio dropout | Load everything before playback starts. |
| Objective-C message dispatch (`objc_msgSend`) | ARC retain/release on audio thread | C++ only. Pure virtual interfaces. |
| Swift function calls | Swift runtime may allocate | Bridge sends commands, polls state. |
| `std::vector::push_back` / any reallocation | Heap alloc | Pre-allocated fixed-size arrays. |
| Exception throw (even if caught) | Stack unwinding cost | No exceptions in audio path code. `-fno-exceptions` for audio translation units. |

**How to think about it:** If you wouldn't do it in an interrupt handler, don't do it in the audio callback.

---

### DOCTRINE 05 — The Runtime Knows Nothing About Yamaha

The Import Adapter converts Yamaha `.sty` → UASF `.uas` **offline**. The Arranger Engine reads only UASF.

**Why this rule exists:**
- Adding Korg/Roland/Ketron import later = writing a new adapter, not touching the engine.
- If Yamaha format changes or we mis-parsed something, we fix the adapter, not the runtime.
- Legal hygiene: reverse engineering a format for interoperability is legally safer than building a runtime system that directly parses it.
- Debugging: you can inspect UASF in any MIDI editor. You cannot inspect a live parse of .sty in a DAW.

**Enforcement:** `ArrangerEngine.h` does not include any file from `ImportAdapter/`. `ImportAdapter/` does not include `ArrangerEngine.h`. They meet only through the `UASFStyle` data model (pure data structs, no logic).

**Articulation metadata rule (per ADR-013):** The runtime engine sees only vendor-neutral articulation profiles (`articulation_profile`, `fidelity_requirement`, `recommended_playback`) in the UASF data model. It does NOT contain any Yamaha-specific parsing, MegaVoice logic, or manufacturer mapping. All Yamaha-specific articulation detection and transformation happens in the Import Adapter (offline), where it is converted to the abstract articulation schema before reaching the runtime.

---

### DOCTRINE 06 — Fail Loud, Degrade Gracefully

**Never be silently wrong.** Every limitation, fallback, or degraded feature must be:
1. Logged with a human-readable message.
2. Visible to the user (UI shows warnings).
3. Auditable in CI (test corpus asserts on limitation counts).

| Import Scenario | Behavior |
|---|---|
| SFF2 guitar NTR → RootTransposition fallback | Logged as warning, shown to user |
| Cross-fill Fill AB → self-fill fallback | Logged as info |
| CASM chunk missing → default bypass NTT | Logged as warning, track plays without chord transform |
| Drum note unmapped → keeps original note | Logged as warning |
| Style file corrupted | Error with clear reason, file rejected, no crash |

**Runtime:** If MIDI device disconnects during a show → engine continues transport (does NOT stop), routes to internal sound if configured, shows red banner. Musician decides when to reconnect.

**MegaVoice fallback policy:** MegaVoice tracks detected during import MUST NOT be silently mapped to GM sounds. The UI MUST display "MegaVoice track detected — high-fidelity playback requires external Yamaha device or premium library." GM fallback is permitted only in MODE 3 (Degraded Preview), with the warning "Low-fidelity fallback mode — NOT suitable for performance" displayed prominently. This degraded mode NEVER counts as a success metric.

**User-Loaded Sound Library errors:** If user-imported SF2/SFZ fails to load (corrupt, unsupported format), the engine MUST log the exact error, fall back to GM without crashing, and show a UI banner indicating the specific sound source that failed. Never be silently wrong.

---

### DOCTRINE 07 — Build Platform, Not App

Every schema, interface, and module boundary must assume future extension. Do not optimize for "just shipping the MVP" if it means painting yourself into a corner.

**Design-for-future rules:**

| Area | MVP Choice | Future-Proofing |
|---|---|---|
| `noteRanges` array | 1 entry (SFF1) | **Schema uses array from v1.0** — SFF2 Ctb2 multi-range drops in without migration |
| `chordType` enum | 34 types declared, 14 recognized by Chord Engine | 34-value enum matches Yamaha CASM 1:1 — no schema migration later |
| `pack.metadata` | `license: "user-imported"`, `dependencies: []` | Complete metadata schema from day 1 for marketplace |
| `IStyleProcessor` interface | No implementation | Interface declared so AI post-process step fits without engine changes |
| MIDI routing | Per-track output config | Schema supports per-track routing with future `AUv3` output type |

**Counter-rule:** Do not add features you don't need "just in case." The future-proofing is in the **schema and interfaces**, not in the implementation. If you don't need multi-range NTR yet, don't implement it — but the JSON model must support it.

---

### DOCTRINE 08 — No DRM Circumvention. Period. + User Sound Consent Policy

**File under "things we do not negotiate":**
- Do not decrypt or attempt to decrypt Yamaha Expansion Packs (`.ppi`, `.cpi`). DMCA §1201 liability is criminal, not civil.
- Do not bundle or distribute any ROM sample from Yamaha, Korg, Roland, or Ketron.
- Do not provide tools or documentation for extracting samples from hardware.
- Do not distribute `.sty` files that are not user-owned.
- Do not bundle style files with unclear provenance.
- Do not bundle any manufacturer-specific SoundFont (Yamaha XG, Korg, Roland, etc.).
- Do not imply in marketing that the app distributes "Genos sounds", "PSR sound packs", or "Tyros voices".

**User-Loaded Sound Consent Policy (per ADR-013):**
- All imported SF2/SFZ/WAV files require user confirmation of ownership/license (ToS clause + first-import dialog).
- User samples MUST be stored in an isolated directory (`Documents/UserSamples/` or `Application Support/UserSamples/`), never mixed with bundled app assets.
- The app MUST request explicit backup consent before including user samples in iCloud or device backup.
- UI MUST clearly distinguish between "User Sound Library" sources and "App Default GM" sources in all sound selection interfaces.

**The legal path:** User-owned file import. User sample import (SFZ/SF2/WAV) for internal sound. Marketplace with creator declarations and DMCA takedown (Phase 3).

---

### DOCTRINE 09 — Test in Production Conditions from Day 1

**"Works on my Mac" is not an acceptable reference.** Every timing number, latency figure, and stability metric must be measured on **real iPad hardware** under conditions that match a live performance.

| Test Condition | Requirement |
|---|---|
| Hardware reference | iPad Air M1 = primary target. iPad Pro M2 = stretch. iPad mini 6 = minimum. |
| Performance mode | Screen brightness 30% (sân khấu tối). Bluetooth keyboard connected. |
| Background load | Import 50 styles in background while playing live. |
| Buffer size | 256 frames @ 44.1kHz (5.8ms). Stress test at 128 frames. |
| External MIDI | USB MIDI interface (iConnectivity mio or class-compliant). BLE MIDI as secondary. |
| Audio output | Headphone out into audio interface for capture. Internal speakers for subjective tests. |

**Automation:** All P0 latency and timing requirements must have automated CI tests. Nightly stress test (30 min offline render + MIDI parse). Weekly hardware benchmark on real iPad.

---

### DOCTRINE 10 — Ship on Time, Ship Stable

**Priorities when something must give:**

| Situation | Action |
|---|---|
| Timeline pressure on MIDI section switch timing | Defer internal sound improvements. MIDI-only + external playback MVP is still valuable to beta. |
| User-loaded SF2/SFZ not ready | Ship with external MIDI only. Sound library import is Phase 2. |
| Internal sound quality not "great" | GM preview is intentionally degraded. Market as "connect your keyboard for best sound." |
| SFF2 support incomplete | Degrade gracefully with clear limitations. SFF1 is 80% of real-world usage. |
| AUv3 hosting incomplete | Phase 2. External MIDI covers professional use cases in Phase 1. |
| SwiftUI animation not pixel-perfect | Ship simple. Musicians care about timing, not animations. |
| Marketplace not ready | Phase 3. MVP = import + play. |
| AI composition | Phase 2+. MVP = arranger, not composer. |
| MegaVoice articulation mapping | Ship with external-Yamaha-mode + GM-fallback-warning. Premium articulation library is Phase 2/3. |

**The only non-negotiable:** Section switch timing accuracy. If it's off by more than 2 ticks (at 480 PPQ, 120 BPM), do not ship.

---

### DOCTRINE 11 — External Playback & User Sound Ecosystem

**The product's sound value is in the ecosystem, not the engine.** The app provides the arranger brain and live UX; sound sources are user-provided or external.

**Ecosystem components:**

| Component | Phase | Description |
|---|---|---|
| External MIDI Device Playback | 1 | Route arranger output to any USB/BLE/Network MIDI tone generator. Highest fidelity. |
| User-Loaded SF2/SFZ Library | 2 | User imports own SoundFont files. Full velocity layering, no manufacturer restrictions. |
| AUv3 Instrument Hosting | 2 | Host third-party instrument plugins within the app. Premium sound via App Store ecosystem. |
| Bundled GM Preview Bank | 1 | Single-layer GM SoundFont (GeneralUser GS or Fluid R3). Preview/practice only. |
| Premium Sound Marketplace | 3 | Creator-published sound packs sold in-app. NOT bundled. Creator-owned content with DMCA process. |

**MegaVoice playback chain (per ADR-013):**
```
Style File (.sty)
  → Import Adapter (detects MegaVoice, produces UASF with articulation_profile="mega_like")
    → UASF Store
      → Load Time (fidelity_requirement check)
        → Playback time:
            IF external Yamaha device detected AND track.routing → Yamaha device:
              MODE 1: External Yamaha Playback (highest fidelity)
              Display: "External Device Playback — Full Articulation"
            ELIF user-loaded premium library available:
              MODE 2: User-Loaded Premium Library
              Display: "User Sound Library — Articulation-Mapped"
            ELSE:
              MODE 3: Degraded Preview (GM fallback)
              Display: "⚠️ Low-fidelity fallback mode — NOT suitable for performance"
              NEVER counted in success metrics
```

**User Sound Library Isolation (per ADR-013):**
- Imported SF2/SFZ stored in `Application Support/UserSamples/`
- Bundled GM bank in app bundle `.app/BundledGM.sf2`
- User samples NEVER mixed with bundled assets
- User consent dialog on first import: "You confirm you own or have license for these samples"
- Backup: explicit opt-in for user samples in iCloud/device backup

**Commercial moat (per ADR-013):**
The product's durable competitive advantage is:
- ✅ Arranger workflow quality (timing, NTT/NTR, section switching)
- ✅ Live performance UX (low-light, one-handed, glanceable)
- ✅ Import compatibility (SFF1/SFF2 → UASF pipeline)
- ✅ Timing quality (sub-tick accuracy)
- ✅ Reliability (zero crash, stable MIDI)
- ✅ Cloud ecosystem (setlists, style library sync)
- ✅ AI-assisted workflow (Phase 2+)
- ✅ Musician productivity (faster setup, better live experience)

**Explicitly NOT commercial moat:**
- ❌ Bundled proprietary sounds
- ❌ Sound realism
- ❌ Hardware ROM emulation
- ❌ "Sounds like Yamaha Genos" claims

---

# B. WORKSTREAM BREAKDOWN (12 WORKSTREAMS)

---

## WS-01: Product & Musician Workflow

**Objective:** Define product requirements, musician personas, UX flows, acceptance criteria, and manage the cross-functional feature backlog. This workstream ensures engineering builds the right thing.

**Scope:**
- Product Requirements Document (PRD) maintenance and prioritization (P0/P1/P2/P3)
- User persona and use case validation (Minh, Hương, Carlos)
- Feature acceptance criteria definition for all user stories (US-01 through US-14)
- Musician workflow design: import → organize → load → play → manage setlist
- Competitive analysis tracking (vArranger, One Man Band, JJazzLab, Camelot Pro)
- Pricing model definition and A/B testing with beta cohort
- Market positioning: "Live arranger for iPad — import your style library, play live"
- Beta feedback protocol design, feedback form creation, community management

**Out of Scope:**
- Engineering architecture decisions (owned by Tech Lead)
- Visual/UI design (owned by Designer, part of WS-06)
- Content creation (style recording/editing)

**Owner Role:** Product Manager

**Dependencies:**
- Input from: WS-06 (iPad Live UX) for feasibility of UX flows
- Input from: WS-09 (Legal/IP) for trademark compliance
- Provides to: All workstreams — prioritized feature backlog

**Deliverables:**
- D-WS01-01: PRD v1.0 (signed off, with P0/P1/P2/P3 matrix)
- D-WS01-02: User persona profiles (Minh, Hương, Carlos) with validated JTBD
- D-WS01-03: Feature backlog (GitHub Issues / Linear) with acceptance criteria
- D-WS01-04: Beta feedback protocol + feedback form templates
- D-WS01-05: Competitive landscape update (monthly)
- D-WS01-06: Pricing recommendation report (pre-launch)

**Risks:**
- R-PM-01: Scope creep — non-MVP features pushed by stakeholders
- R-PM-02: Musician feedback conflicts with technical feasibility
- R-PM-03: Product-market fit assumptions wrong (WTP lower than expected)

**Acceptance Criteria:**
- AC-PM-01: All 14 user stories (US-01 to US-14) have documented acceptance criteria
- AC-PM-02: Beta feedback protocol produces actionable data (not "it's fine")
- AC-PM-03: Feature backlog has clear P0/P1/P2/P3 labels with rationale
- AC-PM-04: Pricing model validated by ≥10 beta musicians

---

## WS-02: Realtime Audio/MIDI Engine

**Objective:** Build the lock-free, sample-accurate audio/MIDI backbone powered by JUCE + CoreAudio + CoreMIDI. This is the "engine room" — everything runs on top of it.

**Scope:**
- JUCE 7 framework integration as C++ audio/MIDI base (Indie license)
- CoreAudio render callback management + AVAudioSession setup
- Lock-free SPSC ring buffer implementation (for commandQueue, chordQueue, outputEventQueue)
- Lock-free double-buffered atomic state snapshot (EngineState for UI poll at 60Hz)
- MusicalClock: audio-callback-driven tick counter, `double` precision, jitter < 1 sample frame
- MIDI Engine: device enumeration, hot-plug (add/remove), per-track routing table
- CoreMIDI integration: MIDI input from keyboard, MIDI output to device (USB/BLE/Network/Virtual)
- MIDI clock out: 24 PPQN, sample-accurate timestamp via MIDITimeStamp
- Virtual MIDI port creation (1 port MVP, all tracks mixed)
- MIDI initialization sequence (GM/GS/XG reset, PC/Bank, CC setup)
- Panic / All Notes Off: flush queue + Note Off + CC123/CC120 on all channels
- Active note tracking (circular buffer, 512 entries)
- Output modes: externalOnly, internalOnly, hybrid, virtualMidi, externalWithInternalFallback
- MIDI send thread (separate from audio thread — MIDISend is not realtime-safe)
- Output sink abstraction (OutputSink interface — enables pluggable routing to MIDI device, internal GM, AUv3 host, or virtual port)
- AUv3 hosting architecture readiness (Audio Unit v3 plugin graph, inter-app audio routing, audio-thread-safe AUv3 communication) — **declared interface, Phase 2 implementation**
- User-loaded sound library routing infrastructure (path for SF2/SFZ samples to reach sfizz renderer, isolated from bundled assets)
- External playback ecosystem support (routing table includes AUv3 output target type, user-library output target type)

**Out of Scope:**
- Sound Engine / sfizz integration (WS-03)
- BLE MIDI latency compensation (document limitations, do not implement complex compensation)
- Full AUv3 host implementation (Phase 2 — architecture readiness only in Phase 1)
- SF2/SFZ file parser (Phase 2 — app loads user-provided SoundFonts via sfizz)

**Owner Role:** Audio/MIDI Engineer

**Dependencies:**
- Depends on: WS-01 for latency requirements (≤ 20ms end-to-end)
- Depends on: ADR-013 for sound source policy and User-Loaded Sound Library architecture
- Provides to: WS-03 (Sound Engine gets event stream), WS-04 (Arranger gets timing substrate), WS-07 (MIDI Device Integration)

**Deliverables:**
- D-WS02-01: JUCE project skeleton with CoreAudio render callback and lock-free queue utilities
- D-WS02-02: MusicalClock implementation (sample-accurate tick counter, double precision)
- D-WS02-03: Lock-free SPSC queue library (commandQueue, chordQueue, eventQueue)
- D-WS02-04: CoreMIDI device enumeration + hot-plug notification
- D-WS02-05: MIDI send thread + output routing table
- D-WS02-06: MIDI initialization sequence engine (GM/GS/XG)
- D-WS02-07: Panic handler (Note Off + CC safety net + flush)
- D-WS02-08: Active note tracker
- D-WS02-09: Latency benchmark suite (hardware MIDI loopback tests)
- D-WS02-10: Virtual MIDI port
- D-WS02-11: OutputSink abstraction interface (pluggable routing to MIDI device / internal GM / AUv3 host / virtual)
- D-WS02-12: AUv3 hosting architecture interface declaration (Phase 1 interface, Phase 2 implementation)
- D-WS02-13: External MIDI playback validation suite (multi-device output, fallback behaviour, latency profile)

**Risks:**
- R-AUDIO-01: CoreAudio callback overrun on older iPad → require buffer fallback
- R-AUDIO-02: Swift/C++ interop overhead in hot path → keep bridge API thin
- R-AUDIO-03: BLE MIDI latency variable → document limitations, do not compensate in v1.0

**Acceptance Criteria:**
- AC-AUDIO-01: Chord → MIDI output latency P95 ≤ 20ms @ 256 buffer (iPad Air M1)
- AC-AUDIO-02: MusicalClock drift < 1 frame per 30 minutes continuous playback
- AC-AUDIO-03: 0 CoreAudio deadline misses in 1h stress test @ 256 buffer
- AC-AUDIO-04: Panic → All Notes Off in ≤ 50ms
- AC-AUDIO-05: MIDI device hot-plug: no crash, transport continues, banner shown
- AC-AUDIO-06: 0 orphan NoteOn after any test scenario

---

## WS-03: Arranger Runtime Logic

**Objective:** Implement the "brain" of the product — the realtime arranger engine that transforms MIDI events based on chord input, manages section state machines, and schedules sample-accurate output. This is the core IP of the entire project.

**Scope:**
- **Section State Machine:**
  - States: STOPPED, COUNT_IN, PLAYING (with current section)
  - Sections: Intro (1-3), Main (A-D), Fill (A-D, cross), Break, Ending (1-3)
  - Quantized section switching: fill = beat boundary, main/ending = bar boundary, intro = immediate
  - Pending section mechanism: UI sets pending → engine commits at next boundary → visual feedback
  - Auto Fill: when switching Main A→B with autoFill=on, engine inserts Fill before destination
  - Ending behavior: runs to completion → STOPPED; double-tap Ending → next Ending ordinal

- **Chord Transformation Pipeline (NTR/NTT):**
  - Note Range selection: determine which noteRange contains the source note
  - Chord Mute / Note Mute filtering
  - NTR (Note Transposition Rule): rootTransposition, rootFixed, guitar (fallback)
  - NTT (Note Transposition Table): bypass, melody, chord, bass, melodicMinor, harmonicMinor, naturalMinor, dorian + B5 variants
  - Note Limit wrapping (octave clamping)
  - Active note tracking (srcNote → playedNote mapping for Note Off matching)

- **Retrigger Semantics:**
  - 6 policies: stop, pitchShift, pitchShiftToRoot, retrigger, retriggerToRoot, noteGenerator (fallback)
  - Apply chord change to active notes immediately within the audio callback
  - chordQuantize mode (off/beat/bar) for delayed chord application

- **Render Loop:**
  - Event iteration within `[blockStartTick, blockEndTick)`
  - MIDI clock pulse emission (24 PPQN, sample-accurate)
  - Catch-up mode for tight buffers (no dropped events)
  - Published EngineState snapshot (atomic, double-buffered)
  - **Articulation-aware rendering:** When track has `articulation_profile` in UASF, render loop respects profile for velocity mapping and note scheduling. The engine reads only vendor-neutral articulation metadata, never manufacturer-specific logic.

- **Command Queue Processing:**
  - Start/Stop (instant or sync), SetSection, Fill, Break, Ending
  - SetTempo, TapTempo, SetAutoFill, SetChordQuantize, Panic
  - Tempo smoothing: 4-tap average with max ±X% change per update

- **Style Loading:**
  - Atomic pointer swap for loaded UASFStyle
  - RCU-style deferred deletion of old style (confirm no audio thread is referencing)
  - Pre-computed NTT lookup tables at load time

**Out of Scope:**
- Chord recognition itself (WS-05 handles that)
- Sound Engine (WS-03 route events to it)
- MIDI routing (WS-02 handles)
- Style file parsing (WS-05 handles SFF1/SFF2→UASF)
- MegaVoice articulation emulation (Phase 2/3 — Phase 1 only detects and preserves in UASF)

**Owner Role:** Audio/MIDI Engineer (same as WS-02 — these are deeply coupled)

**Dependencies:**
- Depends on: WS-02 (realtime timing substrate, MusicalClock)
- Depends on: WS-04 (UASF spec + style loading interface)
- Depends on: ADR-013 (articulation metadata schema, fidelity rules)
- Provides to: WS-07 (MIDI Device Integration receives event stream)
- Provides to: WS-02 (rendering mode decision based on articulation_profile)

**Deliverables:**
- D-WS03-01: MusicalClock implementation (integrated with WS-02 audio callback)
- D-WS03-02: SectionStateMachine with arranger-standard quantized switching (KORG-first validation target; hardware parity unverified — see `docs/strategy/KORG_FIRST_VALIDATION_STRATEGY.md`)
- D-WS03-03: Chord transformation pipeline (NTR + NTT + NoteLimit)
- D-WS03-04: Retrigger engine (6 policies)
- D-WS03-05: Render loop (event iteration + clock out + state publish + articulation-aware scheduling)
- D-WS03-06: Command queue processor (all commands)
- D-WS03-07: Style loader (atomic swap + pre-compute + RCU cleanup)
- D-WS03-08: Panic handler (flush + all notes off)
- D-WS03-09: NTT lookup table generator (offline, chord theory baseline)
- D-WS03-10: Unit test suite (NTR/NTT: 4,000+ cases; state machine: 150+ cases; retrigger: 100+ cases)
- D-WS03-11: Rendering mode resolver (read articulation_profile + fidelity_requirement at load, determine playback mode per track, signal UI)

**Risks:**
- R-ARR-01: NTT table quality insufficient for musical A/B test → iterative refinement with domain expert
- R-ARR-02: Section state machine edge cases (double ending, mid-fill chord change, pending replacement)
- R-ARR-03: Performance of NTT transform in audio callback → precompute tables, keep O(1) lookup
- R-ARR-04: Articulation profile resolution logic adds load-time complexity → keep O(1) metadata lookups, no audio-thread impact

**Acceptance Criteria:**
- AC-ARR-01: Section switch timing error MAX ≤ 2 tick (at 480 PPQ, 120 BPM)
- AC-ARR-02: NTT transform for chord change applied within 1 frame of chord timestamp
- AC-ARR-03: 0 orphan NoteOn after 30 min continuous render with random chord changes
- AC-ARR-04: 1,000+ automated unit tests passing for transform pipeline
- AC-ARR-05: State machine handles all valid section sequences without glitch
- AC-ARR-06: Auto Fill correctly inserts Fill before destination Main
- AC-ARR-07: Rendering mode resolver correctly classifies tracks into MODE 1/2/3 based on articulation metadata and available playback capabilities
- AC-ARR-08: GM fallback mode (MODE 3) for articulation-heavy tracks never counted as success metric
- AC-ARR-09: Articulation profile influences note scheduling only through vendor-neutral metadata — runtime engine has no manufacturer-specific code

---

## WS-04: Internal Style Format (UASF)

**Objective:** Define, implement, and maintain the Universal Arranger Style Format — the single internal format that the entire runtime ecosystem speaks. UASF is the backbone of the platform's interoperability moat.

**Scope:**
- **Format Specification:**
  - `.uas` = ZIP container (deflate, no encryption)
  - `manifest.json` metadata (all sections per UASF spec v0.9)
  - Section MIDI files as SMF Format 0 (standardized at 480 PPQ)
  - `samples/` directory (optional, for Phase 2)
  - `source.bin` preservation (optional, user-toggleable)
  - `preview.m4a` (optional, for marketplace browsing)

- **Schema Design:**
  - uasf.version (formatVersion + minEngineVersion)
  - meta (id, name, genre, tags, author)
  - pack (packId, license, sampleOwnership, dependencies — marketplace-ready from v1.0)
  - music (tempoBpm, timeSignature, ppq)
  - tracks[] (trackId, name, role, midiChannel, isRhythm, defaults)
  - **tracks[].articulation** (per ADR-013 — vendor-neutral articulation metadata, see extension below)
  - sections[] (sectionId, type, ordinal, file, lengthBeats, loop, fillTarget, trackRules[])
  - TrackRule (sourceChord, noteRanges[] with NTR/NTT, retrigger, limits, chordMute, noteMute, breakpointKey)
  - soundMap[] (program, bankMsb/Lsb, gm2Fallback, internalPreset)
  - drumMap (base, overrides[])
  - fx (reverbSend, chorusSend hints)
  - import (sourceFormat, limitations[], sourceHash)

- **Articulation Metadata Extension (ADR-013):**
  ```json
  {
    "track_type": "string",              // "guitar", "bass", "drum", "pad", "brass", "strings", "synth", "vocal", "other"
    "articulation_profile": "string",     // "mega_like", "velocity_layer", "key_switch", "standard_gm", "custom"
    "fidelity_requirement": "string",     // "low" (GM OK), "medium" (custom bank), "high" (articulation-preserving)
    "recommended_playback": [             // ordered by preference
      "external_yamaha",
      "external_midi_device",
      "premium_library",
      "user_soundfont",
      "gm_fallback"
    ],
    "external_device_map": {             // optional — vendor-neutral device hints
      "bank_msb": 0,
      "bank_lsb": 0,
      "program": 0,
      "device_hint": "yamaha_genos"
    },
    "articulation_hints": {              // vendor-neutral expressive intent
      "velocity_layers": 3,
      "note_thresholds": [60, 80, 100],
      "key_switch_range": {"low": 0, "high": 0}
    }
  }
  ```
  - **Runtime rule:** The engine only reads `articulation_profile`, `fidelity_requirement`, and `recommended_playback` — no manufacturer-specific logic.
  - **Import rule:** All articulation-specific detection (MegaVoice, velocity layers, key switches) happens in the Import Adapter (offline) and is converted to this abstract metadata.
  - **Migration:** The articulation fields are optional in the schema — existing UASF v1.0 files without articulation metadata play as standard tracks.

- **Validation Rules (V01-V07 errors, W01-W06 warnings):**
  - V01: Manifest JSON parse failure
  - V02: Unsupported formatVersion
  - V03: Section file missing or corrupt
  - V04: No main section
  - V05: TrackRule references undeclared track
  - V06: Duplicate midiChannel
  - V07: noteRanges overlapping or gapped
  - W01: lengthBeats mismatch with actual SMF duration (pad/trim)
  - W02: Notes outside noteLowLimit/HighLimit in source
  - W03: Unmapped drum notes
  - W04: Missing fill for a main section
  - W05: Import adapter limitations
  - W06: High-fidelity articulation track has no available high-fidelity playback target → GM fallback will be used, user should see warning

- **Versioning & Migration:**
  - semver major.minor formatVersion
  - minor = additive fields (backward-compatible)
  - major = breaking change (requires migration code)
  - migration chạy on library load, writes new file, preserves source.bin
  - Engine declares supportedFormatVersions; rejects too-new files with upgrade prompt

- **Documentation:**
  - Schema JSON schema file (.schema.json)
  - Example manifest.json for SFF1 import, SFF2 import
  - Mapping table: Yamaha Ctab → UASF TrackRule
  - Mapping table: Yamaha chord type enum → UASF chordType enum (34 values)
  - Articulation metadata specification (per ADR-013), with examples of vendor-neutral profiles
  - Fidelity flag semantics: when is articulation_profile high vs. low vs. medium
  - External device mapping guidance for MODE 1 (External Yamaha Playback)

**Out of Scope:**
- Audio-loop style format (Ketron — Phase 3)
- OTS (One Touch Settings) conversion to Performance preset (Phase 2)
- Style editing/recording UI

**Owner Role:** Tech Lead (with input from all engineers)

**Dependencies:**
- Provides foundation for: WS-05 (Import Adapter writes UASF), WS-03 (Arranger reads UASF)
- Depends on: Input from all other workstreams for schema completeness

**Deliverables:**
- D-WS04-01: UASF Spec v1.0 document (complete with all fields, enums, examples, articulation metadata extension per ADR-013)
- D-WS04-02: JSON schema file (manifest.schema.json, including articulation metadata fields)
- D-WS04-03: UASF reader/writer C++ library
- D-WS04-04: ZIP container utility (read/write .uas files)
- D-WS04-05: Validation engine (V01-V07, W01-W06)
- D-WS04-06: Migration framework (version check, migration function registry)
- D-WS04-07: Unit test suite (round-trip: serialize → parse → identical)
- D-WS04-08: 34-value chord type enum definition (shared with Chord Engine and Import Adapter)
- D-WS04-09: Articulation metadata spec (ADR-013 extension) with examples of vendor-neutral profiles
- D-WS04-10: Fidelity flag documentation and rendering mode resolution logic

**Risks:**
- R-UASF-01: Schema changes discovered during SFF2 corpus testing → need v1.1 quickly
- R-UASF-02: source.bin storage bloat for large libraries → warn user at 2GB threshold
- R-UASF-03: Forward-compatibility assumptions wrong → defer unknown fields policy
- R-UASF-04: Articulation metadata schema insufficient for some MegaVoice variants → extension mechanism allows future profile types without schema migration

**Acceptance Criteria:**
- AC-UASF-01: Round-trip test: serialize valid UASF → parse → identical to original
- AC-UASF-02: Validation correctly rejects all V01-V07 scenarios with clear error messages
- AC-UASF-03: Migration from v0.9 → v1.0 works without data loss
- AC-UASF-04: schema supports future SFF2 Ctb2 multi-range (noteRanges array from day 1)
- AC-UASF-05: All 7 validation errors and 6 warnings documented with test fixtures
- AC-UASF-06: Articulation metadata round-trip: MegaVoice profile detected → stored → read → rendering mode resolved correctly
- AC-UASF-07: Optional articulation metadata field — files without articulation data play as standard tracks without error

---

## WS-05: Importer / Compatibility Layer

**Objective:** Build the offline Import Adapter that converts Yamaha SFF1/SFF2 style files to UASF. This is the product's "moat" — compatibility with existing libraries is the primary reason musicians will switch to this platform.

**Scope:**
- **SFF1 Import:**
  - Format detection: magic bytes "SFF1" (or "MThd" legacy fallback)
  - SMF header parse (validate format=0, numTracks=1)
  - Full track parse: section extraction via Marker meta-events (SInt, Intro A/B/C, Main A-D, Fill In AA-DD, Fill In BA-BD, Break, Ending A/B/C)
  - SInt (Style Initialization) parse: CC/PC/Bank/SysEx XG → soundMap + fx hints
  - CASM chunk detection and parse (CSEG → Sdec → Ctab)
  - Ctab field extraction: srcChannel, dstChannel, name, note limits, chordMuteBitmask, noteMuteBitmask, sourceChord root + type, NTR, NTT, breakpointKey, RTR
  - Tick normalization from source PPQ → 480 PPQ
  - Section MIDI extraction: strip tempo/time-sig/markers, pad/trim to exact bar boundary
  - Limitation detection and logging (OTS_SKIPPED, MULTI_TEMPO_STRIPPED, FILL_CROSS_DEGRADED)
  - UASF builder: construct manifest.json, write section .mid files, package zip
  - Validation V01-V07 on output UASF

- **SFF2 Import:**
  - Format detection: magic bytes "SFF2"
  - MIDI track parse (same as SFF1 for section extraction)
  - CASM chunk parse with Ctb2 detection (multi-range noteRanges)
  - Ctb2 → UASF: multiple noteRanges entries (up to 3), extended NTT enum (naturalMinor, dorian, B5 variants)
  - Guitar NTR fallback: map to rootTransposition + limitation SFF2_GUITAR_NTR_DEGRADED
  - **MegaVoice Detection & Preservation (per ADR-013):**
    - Detect MegaVoice tracks: velocity range > 2 layers, note range spans > 40 keys, specific NTR patterns
    - Do NOT silently map to GM — preserve articulation metadata in UASF with `articulation_profile: "mega_like"`
    - Set `fidelity_requirement: "high"` and `recommended_playback: ["external_yamaha", "premium_library", "user_soundfont"]`
    - Generate articulation hints (velocity thresholds, key switch ranges) for external device playback
    - Limitation code: SFF2_MEGAVOICE_DETECTED (informational — NOT a degradation)
  - **Unsupported Articulation Handling:** Tracks with articulation patterns not representable in current schema → limitation code SFF2_UNSUPPORTED_ARTICULATION + preserved as standard MIDI with fidelity_requirement warning
  - Graceful degradation: MIDI section data still works even if CASM parse partial

- **User-Loaded Sound Library Support** (architecture readiness):
  - Import pipeline is format-agnostic: new importers (SF2, SFZ) are separate adapters that write UASF-compatible metadata
  - UASF articulation fields provide the metadata bridge between style tracks and user-loaded sound banks
  - External MIDI device playback is the primary high-fidelity path in Phase 1
  - User-loaded SF2/SFZ import and articulation mapping is Phase 2

- **Test Corpus Strategy:**
  - Collect 500+ style files from beta musicians (user-owned, legally obtained)
  - Diversity: PSR-E/S-series, Tyros 1-5, Genos; SFF1 + SFF2; multiple genres
  - Automated nightly test: import entire corpus, track success/warning/error rates
  - Regression test on 50-style fast subset per commit
  - A/B test: compare playback with Yamaha reference (recorded audio)

- **Error Handling:**
  - Clear, actionable error messages (not error codes)
  - Human-readable limitation descriptions with musical impact
  - Per-style import result (success + warnings) and per-batch summary

**Out of Scope:**
- Yamaha Expansion Pack (.ppi/.cpi) — research only, NO implementation (DMCA §1201)
- Korg/Roland/Ketron import — Phase 2/3
- .sst bundle unpack — Phase 2
- AI-assisted remap — Phase 2+

**Owner Role:** Audio/MIDI Engineer (same person — parser is C++ core engine)

**Dependencies:**
- Depends on: WS-04 (UASF spec for output format)
- Provides to: WS-01 (import flow UX requirements), WS-08 (test corpus for harness)

**Deliverables:**
- D-WS05-01: SFF1 binary parser (magic → sections → CASM → Ctab all fields)
- D-WS05-02: SFF2 binary parser (Ctb2 multi-range + extended NTT)
- D-WS05-03: UASF builder (manifest.json generation + SMF writer + zip packager + articulation metadata population)
- D-WS05-04: Tick normalizer (source PPQ → 480 PPQ with rounding < 0.1ms error)
- D-WS05-05: ImportResult type + limitation log system (including SFF2_MEGAVOICE_DETECTED, SFF2_UNSUPPORTED_ARTICULATION)
- D-WS05-06: Drum note mapper (Yamaha note → GM with override table)
- D-WS05-07: Sound map builder (Yamaha voice → GM program lookup table)
- D-WS05-08: Test corpus 500+ styles with automated import test runner
- D-WS05-09: Yamaha→UASF bit-level mapping documentation (validated by Sprint 0 spike)
- D-WS05-10: A/B test harness: render UASF + compare with Yamaha reference
- D-WS05-11: MegaVoice detection matrix — algorithm for detecting MegaVoice tracks from velocity range, note range, NTR patterns
- D-WS05-12: Articulation metadata builder — populates UASF articulation_profile, fidelity_requirement, recommended_playback, articulation_hints from detected patterns
- D-WS05-13: Unsupported articulation handling — tracks with unrecognised articulation patterns get fidelity_requirement warning + standard MIDI preservation

**Risks:**
- R-IMP-01: CASM Ctab field offsets wrong (based on reverse-engineering) → Sprint 0 hex dump + JJazzLab cross-ref
- R-IMP-02: SFF2 Ctb2 multi-range encoding position uncertain → corpus validation
- R-IMP-03: Test corpus quality — must ensure all files user-owned, no copyright issues
- R-IMP-04: SFF2 import success rate < 75% → adjust target downwards for v1.0, publish known limitation list
- R-IMP-05: MegaVoice detection false positives (standard tracks flagged as articulation-heavy) → tune detection thresholds during corpus validation; false negatives are preferable to false positives
- R-IMP-06: Unsupported articulation patterns in Genos-specific styles → flag as SFF2_UNSUPPORTED_ARTICULATION with clear user guidance

**Acceptance Criteria:**
- AC-IMP-01: SFF1 import success rate ≥ 95% on 500-style corpus
- AC-IMP-02: SFF2 import success rate ≥ 75% (MIDI sections readable, CASM where possible)
- AC-IMP-03: 0 crashes on any file in corpus (graceful rejection for unsupported formats)
- AC-IMP-04: All limitation codes produce human-readable messages (English + Vietnamese)
- AC-IMP-05: Tick normalization error < 1 tick at 480 PPQ for all source PPQ values
- AC-IMP-06: Import of a typical 100KB SFF1 file completes in < 2 seconds (iPad Air M1)
- AC-IMP-07: MegaVoice detection marks tracks with articulation_profile="mega_like" — no false negatives on corpus MegaVoice tracks
- AC-IMP-08: Imported MegaVoice tracks play back correctly via MODE 1 (external Yamaha device) and show correct "Low-fidelity fallback" warning in MODE 3
- AC-IMP-09: Unsupported articulation tracks produce SFF2_UNSUPPORTED_ARTICULATION limitation with actionable message

---

## WS-06: iPad Live UX

**Objective:** Design and implement the entire user interface — focused on the "live first" principle. Every design decision must answer: "Can a musician on stage, in low light, with one hand free, operate this?"

**Scope:**
- **Live Screen UI (highest priority):**
  - Section pad grid: Intro (1-3), Main (A-D), Fill (A-D), Break, Ending (1-3)
  - States per pad: Default / Active (cyan) / Pending (amber pulsing, 0.8Hz) / Disabled
  - Chord display: 64pt bold, full width, always visible
  - Bar/beat indicator
  - Tempo display (tap-to-edit) + tap tempo button
  - Transpose control
  - Panic button: 88×88pt minimum, always visible, no confirm dialog
  - Style name display
  - Bottom bar: Start/Stop transport + tempo + transpose + panic

- **Performance Mode:**
  - 3-finger tap to activate/deactivate
  - When active: hides tab bar, disables navigation gestures, only live controls active
  - Lock icon visible in corner

- **Mixer Panel:**
  - Slide from right, collapsible
  - Per-track: volume fader, pan, mute, solo, reverb/chorus send
  - Routing display (USB:channel / INT / VIRT)
  - Master volume + reverb/chorus master

- **Style Browser:**
  - List + grid view
  - Real-time search via SQLite FTS5 (< 200ms response)
  - Filter by genre, tempo range, format (SFF1/SFF2), favorites, warnings
  - Sort by name, import date, last used
  - Preview: play Main A 4 bars

- **Setlist Management:**
  - Create/edit/delete setlists
  - Add songs: assign style + tempo + transpose + notes + mixer snapshot
  - Drag-to-reorder
  - Next-song preload mechanism
  - Performance snapshot: save all engine state, restore on load

- **Import Flow:**
  - Multi-file picker from Files.app
  - Background import progress
  - Per-file result sheet: errors/warnings/infos with human-readable descriptions
  - Batch summary

- **On-screen Chord Pad:**
  - Grid layout: 12 roots (horizontal) × 8 types (vertical) = 96 buttons
  - Tap root → hold, tap type → chord

- **Visual Design System:**
  - Dark mode default (#0D0D0F background)
  - Color palette: cyan accent (#00E5FF), amber pending (#FFB300), red panic only (#FF3B30)
  - Typography: SF Pro Rounded Bold 64pt (chord), SF Pro Display Semibold 24pt (section labels)
  - Minimum tap target: 60×60pt (live), 44×44pt (secondary)
  - Section pads: 80×64pt (landscape), 60×48pt (portrait)

- **Layout:**
  - Landscape primary (full section grid visible)
  - Portrait secondary (scrollable, Main A-D + Fill always visible)
  - Navigation: Live tab (default, no back button) + Library + Setlist + Settings
  - Modals: Import, Style Detail, Performance Editor

**Out of Scope:**
- External display support (Phase 2)
- VoiceOver full support (basic VoiceOver labels only in MVP)
- iPhone-specific optimization (app runs but not fully optimized)
- Fancy animations > 300ms

**Owner Role:** Swift/iOS Engineer + Designer (Part-time)

**Dependencies:**
- Depends on: WS-02/WS-03 (AbstractCommand interface + EngineState struct definition)
- Depends on: WS-04 (library data model for Style Browser)
- Provides to: WS-01 (UX validation with beta musicians)

**Deliverables:**
- D-WS06-01: Live Screen UI (section pads, chord display, transport, panic)
- D-WS06-02: Performance mode (3-finger gesture lock, simplified interface)
- D-WS06-03: Mixer panel (collapsible, per-track controls, routing)
- D-WS06-04: Style Browser (search, filter, sort, preview)
- D-WS06-05: Setlist management (CRUD, drag-reorder, preload)
- D-WS06-06: Import flow (Files picker, progress, result sheet)
- D-WS06-07: On-screen chord pad (12×8 grid)
- D-WS06-08: Visual design system (color palette, typography, component library)
- D-WS06-09: Haptic feedback integration (section tap, commit, panic, error)
- D-WS06-10: Error presentation system (toast/banner/modal/full-overlay)

**Risks:**
- R-UX-01: SwiftUI performance with 60Hz EngineState polling → throttle updates, use diffing
- R-UX-02: Section grid scroll in portrait vs landscape → responsive layout with SwiftUI's SizeClass
- R-UX-03: On-screen chord pad usability with 96 small buttons → test with beta; consider alternative layouts

**Acceptance Criteria:**
- AC-UX-01: All section pads ≥ 60×60pt, with correct state rendering (default/active/pending/disabled)
- AC-UX-02: Chord display: 64pt font, readable from 1m in low light
- AC-UX-03: Performance mode: 3-finger activation, no navigation leaks, visible lock icon
- AC-UX-04: Panic button: 88×88pt, always visible, no confirm dialog
- AC-UX-05: Style search: response < 200ms with 500+ styles
- AC-UX-06: Import batch of 20 styles: shows progress, per-file results, batch summary
- AC-UX-07: On-screen chord pad: chord emitted within 100ms of tap

---

## WS-07: MIDI Device Integration

**Objective:** Ensure the app works reliably with the widest possible range of MIDI hardware — USB, BLE, Network, and Virtual MIDI — under real stage conditions.

**Scope:**
- **USB MIDI (highest priority):**
  - Class-compliant USB MIDI devices
  - CoreMIDI device enumeration + hot-plug notification (add/remove)
  - Auto-connect: detect known devices, configure routing
  - Manual device selection in settings
  - Latency budget: USB MIDI ≤ 3ms end-to-end

- **BLE MIDI:**
  - Apple BLE MIDI support (CoreMIDI standard)
  - Connection management (pairing, reconnection)
  - Latency warning when user selects BLE: "BLE MIDI has higher latency than USB. Recommended for casual use only."

- **Network MIDI (RTP-MIDI):**
  - CoreMIDI Network MIDI session support
  - Display available sessions, manual connect

- **Virtual MIDI Port:**
  - 1 virtual output port (MVP): named after app, visible to AUM/GarageBand/Drambo
  - Option for virtual input port (for MIDI from DAW into arranger)
  - Session management (advertise service, respond to client connections)

- **Device Routing System:**
  - Per-track routing: {trackId, outputTarget: {type: "device|internal|virtual", deviceId, channel}}
  - Performance preset stores routing configuration
  - Fallback mode: device disconnects → auto-switch to internal + red banner

- **MIDI CC Learn & Map:**
  - User-configurable CC mapping: foot controller → volume, expression, fill trigger (Phase 2, declare interface)
  - Sustain pedal (CC64) → chord engine hold mode toggle

- **Device Initialization:**
  - GM/GS/XG mode selectable per device in settings
  - Init sequence: Bank select → Program Change → Volume → Pan → Reverb/Chorus send → Expression → Sustain off
  - Reconnect: re-send full init sequence after device reconnects

- **Latency Compensation:**
  - No automatic compensation in v1.0 — document measured latency per device type
  - BLE: accept 5-15ms variable latency, warn user

**Out of Scope:**
- MIDI Clock IN (sync from external sequencer) — Phase 2
- SysEx forwarding from keyboard into style output — Phase 2
- Multi-port virtual MIDI (16 ports) — Phase 2

**Owner Role:** Audio/MIDI Engineer + Swift/iOS Engineer

**Dependencies:**
- Depends on: WS-02 (CoreMIDI infrastructure, send thread)
- Depends on: WS-03 (event stream from Arranger Engine)
- Provides to: WS-06 (device selection UI, routing UX)

**Deliverables:**
- D-WS07-01: USB MIDI integration (enumeration, connect, send/receive)
- D-WS07-02: BLE MIDI integration with latency warning
- D-WS07-03: Network MIDI integration (RTP-MIDI session)
- D-WS07-04: Virtual MIDI port (output + input)
- D-WS07-05: Device initialization sequence (GM/GS/XG + reset + voice setup)
- D-WS07-06: Hot-plug notification + reconnect with init sequence replay
- D-WS07-07: Device routing configuration (per track per performance)
- D-WS07-08: Fallback mode on disconnect (auto-switch to internal + red banner)
- D-WS07-09: Hardware compatibility test report (tested with 10+ MIDI devices)
- D-WS07-10: Sustain pedal (CC64) → chord hold integration

**Risks:**
- R-MIDI-01: BLE MIDI latency unpredictable → warn users, no compensation in v1.0
- R-MIDI-02: Hot-plug race condition (connect during audio callback) → use dispatch queue, never callback
- R-MIDI-03: Random iOS CoreMIDI bugs on specific iPadOS versions → tested matrix

**Acceptance Criteria:**
- AC-MIDI-01: USB MIDI: plug-and-play on all class-compliant devices, no configuration needed
- AC-MIDI-02: Device disconnect during playback: transport continues, banner shows, fallback activates
- AC-MIDI-03: Device reconnect: full init sequence sent, playback resumes within 100ms
- AC-MIDI-04: Virtual MIDI port visible to AUM and GarageBand on same device
- AC-MIDI-05: Per-track routing works correctly in all 4 modes (external/internal/hybrid/virtual)
- AC-MIDI-06: Sustain pedal CC64 correctly toggles chord hold mode
- AC-MIDI-07: Zero orphan notes after any disconnect/reconnect sequence

---

## WS-08: Test Harness / Timing Lab

**Objective:** Build the testing infrastructure that validates realtime timing, latency, stability, and sound quality before any musician touches the app. This is not optional — testing is the release gate.

**Scope:**
- **Unit Test Framework:**
  - GoogleTest for C++ engine tests (linked only in test targets, not shipped)
  - XCTest for Swift tests
  - All test cases run on every commit (target: < 5 min)
  - 100% pass required before merge

- **Chord Engine Unit Tests (900+ cases):**
  - 408 exact match (34 types × 12 roots)
  - 336 inversion tests (14 MVP types × 12 roots × 2 inversions)
  - 50 partial match (missing note, extra note)
  - 30 single finger (Yamaha conventions)
  - 24 slash chord
  - 15 hold mode
  - Fuzz: 100,000 random inputs, 0 crash invariant

- **Arranger Engine Unit Tests (4,000+ cases):**
  - 1,440 NTR rootTransposition cases
  - 1,440 NTR rootFixed cases
  - 60 noteLimit wrap cases
  - 2,448 NTT transform cases (6 modes × 34 types × 12 inputs)
  - 100+ retrigger cases (6 policies × chord change scenarios)
  - 150+ state machine cases (every valid/invalid section sequence)
  - Invariants: output ∈ [0,127], NoteOn count == NoteOff count, monotonic timestamps

- **Integration Tests:**
  - Full pipeline: SFF1 → UASF → Arranger → MIDI output (5 style smoke test)
  - Panic test: 20 notes → Panic → 0 active notes + all channels flushed
  - MIDI disconnect test: transport continues, fallback activates, reconnect sends init
  - Section switch timing precision (offline render + MIDI parse)

- **Timing Stress Test (Most Critical):**
  - 30-minute offline render with 5 styles cycling
  - Random chord changes every 2-8 beats, section switches every 4-16 bars
  - Background CPU load injection (import 50 styles concurrently)
  - Metric: P50 < 0.5 tick, P99 < 2 tick, MAX < 5 tick (FAIL if > 5 tick any single event)
  - Orphan note detection: scan full output, must be 0

- **Memory Stability Test:**
  - 2-hour session: import 20 styles, play live, switch style 50 times, panic 10 times
  - Memory leak detection: usage after 2h must be ≤ baseline × 1.1

- **Hardware Latency Benchmark:**
  - Hardware MIDI loopback (iConnectivity mio or equivalent)
  - 1,000 trials per test scenario
  - Report P50/P95/P99 for chord latency, section switch timing, panic response
  - Test on iPad Air M1 (primary), iPad Pro M2 (stretch), iPad mini 6 (minimum)

- **A/B Musicality Tests:**
  - Render same style + chord sequence through app and Yamaha reference keyboard
  - Domain expert (musician) rates similarity 1-5
  - NTT table refinement target: average score ≥ 3.5/5 for MVP

- **CI/CD Pipeline:**
  - Build + unit tests on every commit (< 5 min)
  - Integration tests on every merge to main (< 10 min)
  - Full corpus import test (nightly)
  - Timing stress test (nightly)
  - Memory stability test (nightly)
  - Regression fixture commit policy: every bug fix includes fixture

**Out of Scope:**
- Full XCUITest UI automation (Phase 2 — after UI stabilizes)
- Load testing (no server-side component in MVP)
- 100% code coverage (focus on behavior coverage of P0 requirements)

**Owner Role:** QA/Timing Engineer

**Dependencies:**
- Depends on: WS-02, WS-03, WS-04, WS-05 (engine modules to test)
- Provides to: All workstreams — test pass/fail signals for release gates

**Deliverables:**
- D-WS08-01: CI pipeline configuration (Xcode Cloud or GitHub Actions + self-hosted Mac)
- D-WS08-02: GoogleTest + XCTest integration
- D-WS08-03: Chord Engine test suite (900+ automated cases)
- D-WS08-04: NTR/NTT transform test suite (4,000+ automated cases)
- D-WS08-05: State machine test suite (150+ automated cases)
- D-WS08-06: Timing stress test harness (offline render + MIDI parse + timing analysis)
- D-WS08-07: Memory stability test harness (2h session + allocation tracking)
- D-WS08-08: Hardware latency benchmark script (logs to CSV for trend tracking)
- D-WS08-09: A/B musicality test protocol + scoring framework
- D-WS08-10: CI nightly report template (import rates, timing trends, memory stats)

**Risks:**
- R-TEST-01: Timing stress test flaky on CI Mac (not real iPad) → must run on iPad for final validation
- R-TEST-02: MIDI loopback hardware procurement delay → software loopback for early development
- R-TEST-03: Test corpus grows too large for CI storage → Git LFS or S3 with subset for fast CI

**Acceptance Criteria:**
- AC-TEST-01: CI runs full test suite in < 5 min; all tests pass before any merge
- AC-TEST-02: Timing stress test passes: P99 < 2 tick, MAX < 5 tick, 0 orphan notes
- AC-TEST-03: Memory stability test passes: 2h session, leak < 10% growth
- AC-TEST-04: Nightly corpus test: ≥ 95% SFF1, ≥ 75% SFF2 import success
- AC-TEST-05: Regression fixture committed with each bug fix
- AC-TEST-06: Hardware latency benchmarks measurable and tracked weekly

---

## WS-09: Legal / IP / Compliance

**Objective:** Ensure the product is legally safe to ship. No DMCA violations. Clean trademark. Proper OSS compliance. App Store review-ready ToS.

**Scope:**
- **Format Reverse-Engineering Legal Analysis:**
  - Document legal precedent supporting .sty parsing for interoperability
  - US: Sega v. Accolade (9th Cir. 1992), Sony v. Connectix (9th Cir. 2000), Google v. Oracle (SCOTUS 2021)
  - EU: Software Directive Article 6 (2009/24/EC)
  - Reference: JJazzLab (open-source), vArranger (commercial) — established practice
  - Policy: parse user-owned files only, no distribution of .sty files

- **DMCA Compliance (Critical):**
  - Review: .sty files are NOT encrypted → no circumvention needed
  - ⛔ Expansion Pack (.ppi/.cpi): ABSOLUTELY FORBIDDEN to circumvent encryption
  - Policy document: "No DRM Circumvention Policy" signed by all engineers

- **Trademark (Highest Legal Risk):**
  - App name trademark search: USPTO, EUIPO, JPO (budget $500-2,000)
  - Nominative fair use rules: {"imports styles in Yamaha® .STY format"} ✅
  - Prohibited: {"Yamaha Arranger", "Genos App", "Korg Arranger", any logo, any endorsement claim}
  - App Store description: include "not affiliated with" disclaimer
  - Marketing copy review: all comparisons factual, not misleading

- **Open Source License Compliance:**
  - sfizz (BSD-2): include copyright notice in About screen
  - Airwindows (MIT): include copyright notice
  - GRDB.swift (MIT): include copyright notice
  - GoogleTest (BSD-3): test-only, do not ship
  - ⛔ No GPL-3 dependencies in production target (especially zita-rev1, MuseScore GM soundfont)
  - ⛔ No FluidSynth (LGPL dynamic-link issues on iOS App Store)
  - Create `THIRD_PARTY_LICENSES.txt` accessible from Settings/About
  - CI audit script: detect new dependency licenses before merge

- **Soundfont Licensing:**
  - GeneralUser GS (custom free, allow distribution, attribution required): SELECTED
  - Fluid R3 GM (MIT/LGPL dual): potential alternative
  - Verify current license text before bundling
  - ⛔ Yamaha XG soundfont (any): proprietary, do not use

- **Terms of Service (ToS) Draft:**
  - File Import clause: "User warrants they own or have rights to imported files"
  - Sound Samples clause: "App does not bundle/assist extracting manufacturer ROM samples"
  - Manufacturer Trademarks clause: "Not affiliated with/disclaimers"
  - Marketplace clause (placeholder): "Creator warranty + takedown process"

- **App Store Review Readiness:**
  - Privacy Policy: app is offline, no data collection MVP
  - MIDI permission request: explain clearly in purpose string
  - Copyright review: no bundled proprietary content
  - No analytics/tracking in MVP (opt-in crash reporting post-Privacy Policy)

**Out of Scope:**
- Formal legal representation (budget for trademark search + ToS review only: ~$3,000-5,000)
- Patent search (low risk — algorithms are well-established)
- DMCA agent registration (Phase 3 — marketplace launch)

**Owner Role:** Legal/IP Counsel (contractor) + Founder

**Dependencies:**
- Provides to: WS-12 (Commercial Packaging — App Store compliance)
- Provides to: WS-01 (marketing copy, pricing)
- Depends on: WS-04 (UASF license field design input)

**Deliverables:**
- D-WS09-01: IP Risk Assessment document (final, with all 16 risk items analyzed — including 2 new HIGH risks from ADR-013)
- D-WS09-02: Reverse-engineering legal memo (format .sty interoperability)
- D-WS09-03: "No DRM Circumvention Policy" (signed by team)
- D-WS09-04: Trademark search report (USPTO + EUIPO + JPO)
- D-WS09-05: App name approval (trademark-clear name)
- D-WS09-06: ToS draft (privacy, import liability, trademarks, DMCA placeholder, user-uploaded sound consent clause)
- D-WS09-07: OSS license audit + THIRD_PARTY_LICENSES.txt
- D-WS09-08: GeneralUser GS license verification (email author, confirm bundling OK)
- D-WS09-09: App Store submission review checklist (trademark, copyright, permissions, sample bundling)
- D-WS09-10: Marketing copy review (no misleading claims, no trademark violation) — includes ADR-013 terminology compliance check
- D-WS09-11: Bundled assets audit — confirmed no proprietary manufacturer samples in app binary (checklist signed by Tech Lead)
- D-WS09-12: ADR-013 terminology compliance review of all marketing and documentation copy

**Risks:**
- R-LEGAL-01: Trademark conflict forces app name change before launch → have 3 backup names ready
- R-LEGAL-02: GeneralUser GS license terms changed → find alternative GM soundfont (Fluid R3 MIT)
- R-LEGAL-03: Apple rejects app for "format compatibility" claim → reference precedent apps (JJazzLab, vArranger)
- R-LEGAL-04: Yamaha sends C&D despite nominative fair use → prepare legal response with interoperability defense
- R-LEGAL-05: **HIGH** — Bundling proprietary Yamaha/Korg sample content (even accidentally) → Mitigation: user-loaded only policy (ADR-013), CI audit of bundled assets, no manufacturer SoundFonts in app binary
- R-LEGAL-06: **HIGH** — Marketing language implying official sound distribution ("Genos sounds included", "PSR sound pack", etc.) → Mitigation: vendor-neutral terminology enforced in all docs, marketing review gate for all copy
- R-LEGAL-07: User-uploaded SF2/SFZ files containing copyrighted samples → Mitigation: ToS clause + consent dialog on first import, DMCA takedown process for marketplace (Phase 3)
- R-LEGAL-08: App Store rejection for bundled sample size or OSS license conflicts with sfizz → Mitigation: bundled GM bank < 50MB, sfizz BSD-2 license verified App Store-compatible

**Acceptance Criteria:**
- AC-LEGAL-01: App name trademark-clear in US, EU, JP
- AC-LEGAL-02: ToS covers all required clauses (import, samples, trademarks, DMCA, user-uploaded sound consent)
- AC-LEGAL-03: THIRD_PARTY_LICENSES.txt complete with all dependencies
- AC-LEGAL-04: App Store description reviewed for trademark compliance — no "Genos sounds", "PSR sound pack", "Tyros voices" anywhere in commercial copy
- AC-LEGAL-05: All engineers have signed No DRM Circumvention Policy
- AC-LEGAL-06: OSS audit CI script prevents GPL/AGPL dependency from entering production
- AC-LEGAL-07: Bundled assets audit: confirmed no proprietary manufacturer samples in app binary
- AC-LEGAL-08: Marketing copy review completed against ADR-013 terminology checklist

---

## WS-10: DevOps / Build / CI / Release

**Objective:** Set up the infrastructure to build, test, and ship the app reliably. Fast CI feedback, reproducible builds, and a smooth TestFlight → App Store pipeline.

**Scope:**
- **Build System:**
  - Xcode project with 5 targets: LiveArranger (iOS), LiveArrangerTests (iOS), LiveArrangerUITests (iOS), CoreEngineTests (macOS — fast C++ tests), SoundfontConverter (macOS CLI)
  - CMake for C++ dependencies (JUCE, sfizz, GoogleTest)
  - Swift Package Manager for Swift dependencies (GRDB.swift)
  - Manual dependency management for JUCE (CMake support) + Airwindows (copy files)
  - ⛔ No CocoaPods. No Carthage.

- **CI Pipeline:**
  - GitHub Actions + self-hosted Mac mini (M-series) for build + test
  - Fast CI (every commit): build debug + release, unit tests, 50-style corpus import test
  - Nightly: full 500-style corpus, timing stress test, memory stability, performance benchmark
  - CI gates: all tests must pass before merge; linting (SwiftLint + clang-tidy) must pass

- **C++ Build Configuration:**
  - Debug: assertions on, ASan + TSan, verbose logging
  - Release: O2, -ffast-math (audio), -fno-exceptions (audio path), NDEBUG
  - Static linking for JUCE + sfizz (App Store compliant)

- **TestFlight Distribution:**
  - Internal testing group (team + close friends)
  - External testing group (20 beta musicians)
  - Build versioning: semantic version + build number + git hash
  - Automated TestFlight upload via CI (post-merge to main)

- **Crash Reporting:**
  - Crashlytics (Firebase) integration
  - Opt-in during beta onboarding
  - IP anonymization enabled
  - Daily crash review by QA during beta

- **Release Pipeline:**
  - Pre-submit checklist (trademark review, ToS, screenshots, metadata)
  - App Store Connect configuration
  - App Store submission preparation (screenshots, description, keywords, privacy URLs)
  - Hotfix process: emergency build within 24h for P0 bugs

- **Code Quality:**
  - SwiftLint rules: strict mode for Swift files
  - clang-tidy for C++ files (focus on audio-thread safety rules)
  - ADR (Architecture Decision Record) repository — every decision documented
  - Code review requirement: minimum 1 approval before merge

**Out of Scope:**
- Cloud backend infrastructure (Phase 3 — marketplace)
- Automated UI testing (XCUITest — Phase 2)
- Performance monitoring in production (Phase 2+)

**Owner Role:** DevOps/Release Engineer

**Dependencies:**
- Depends on: All workstreams — builds their code
- Provides to: All workstreams — CI feedback, build artifacts

**Deliverables:**
- D-WS10-01: Xcode project with all targets configured
- D-WS10-02: CMake build system for C++ dependencies
- D-WS10-03: GitHub Actions CI pipeline (fast + nightly)
- D-WS10-04: TestFlight configuration (internal + external groups)
- D-WS10-05: Crashlytics integration (opt-in)
- D-WS10-06: SwiftLint + clang-tidy configuration
- D-WS10-07: App Store Connect metadata (screenshots, description, privacy URLs)
- D-WS10-08: Build versioning scheme + automated TestFlight upload
- D-WS10-09: Hotfix process document
- D-WS10-10: CI nightly report template

**Risks:**
- R-DEVOPS-01: Self-hosted Mac mini unavailable → use Xcode Cloud (slower, capped)
- R-DEVOPS-02: JUCE license upgrade cost when revenue exceeds $50k/year → plan for Pro upgrade
- R-DEVOPS-03: Xcode version mismatch between dev machines and CI → lock Xcode version

**Acceptance Criteria:**
- AC-DEVOPS-01: Full CI build + test in < 10 min (commit CI)
- AC-DEVOPS-02: Nightly full corpus test runs unattended, reports pass/fail
- AC-DEVOPS-03: TestFlight build automatically uploaded after each merge to main
- AC-DEVOPS-04: Crashlytics reports crashes within 24h of beta use
- AC-DEVOPS-05: All engineers can build and run full test suite locally
- AC-DEVOPS-06: Release checklist complete before each App Store submission

---

## WS-11: QA / Musician Validation

**Objective:** Validate that the app actually works for musicians in real performance conditions. This is the bridge between "works in the lab" and "works on stage."

**Scope:**
- **Pre-Beta Quality Gates:**
  - Internal QA: execute all P0/P1 acceptance criteria from every workstream
  - 2h stability test × 3 runs (internal team acts as musician)
  - Corpus test 500 styles: generate report with import rates, warning distribution
  - Performance benchmark: all latency and CPU metrics measured on reference hardware
  - Regression check: known bugs list, verify no reintroductions

- **Beta Program Management:**
  - Recruit 20 beta musicians via:
    - Facebook groups (nhạc công VN)
    - Yamaha community forums (international)
    - Personal network (domain expert connections)
  - Profile diversity: 8 wedding/event musicians VN, 4 church musicians, 4 international forum users, 4 semi-pro/studio
  - Onboarding: TestFlight invite + welcome email with instructions
  - Support channel: dedicated Discord/Telegram group for rapid feedback

- **Beta Feedback Protocol:**
  - Automated: Crashlytics crash reporting (opt-in, anonymous)
  - Post-show form (5 questions, < 2 min):
    1. Crash/glitch? Describe.
    2. Style import issues?
    3. Section switch lag/off-beat?
    4. Chord recognition satisfaction? (Never / Rarely / Sometimes / Often / Always)
    5. Overall score 1-5 vs. your current hardware/software
  - Bi-weekly: video call with cohort (listening session, deep feedback)
  - Weekly: beta coordinator produces feedback summary report

- **Bug Triage Process:**
  - P0: Show-stopper (crash, timing off, orphan notes) → fix within 24h
  - P1: Major (wrong notes, import regression) → fix within beta cycle (2 weeks)
  - P2: Minor (UI polish, missing warning text) → fix before launch
  - P3: Enhancement → backlog

- **Musician Validation Sessions:**
  - Domain expert (in-house musician) does weekly A/B tests: app vs. Yamaha reference
  - A/B score target: ≥ 3.5/5 for NTT transform accuracy
  - Focused testing: chord recognition edge cases, single finger mode, auto fill behavior
  - Create "musician bug report" template (non-technical language)

- **Corpus Expansion:**
  - Collect additional styles from beta musicians (user-owned, consent obtained)
  - Add to test corpus (with source attribution for internal tracking)
  - Track import success per-model to identify model-specific issues

**Out of Scope:**
- Formal usability lab testing (in-field observation is better for live music apps)
- User acceptance testing (UAT) by customer — beta is sufficient
- Performance testing in non-musical conditions (focus on live scenarios)

**Owner Role:** QA/Timing Engineer + Domain Expert (Musician)

**Dependencies:**
- Depends on: All workstreams — testable builds
- Provides to: WS-01 (beta feedback for product decisions), WS-03 (NTT refinement)

**Deliverables:**
- D-WS11-01: Pre-beta QA checklist (all P0/P1 acceptance criteria)
- D-WS11-02: Beta program plan (recruitment, onboarding, support)
- D-WS11-03: Beta feedback form (5-question post-show)
- D-WS11-04: Bug triage process document (P0/P1/P2/P3 definitions + SLAs)
- D-WS11-05: Weekly beta feedback report template
- D-WS11-06: A/B musicality test protocol + scoring sheets
- D-WS11-07: Known limitations document (for musicians, plain language)
- D-WS11-08: Exit beta criteria checklist (must all pass before launch)
- D-WS11-09: Corpus test report (weekly trend: import rates)
- D-WS11-10: 2h stability test log (each run, each device)

**Risks:**
- R-QA-01: Beta musicians not sufficiently active → set minimum participation requirement (3 shows)
- R-QA-02: Bug reports lack detail → provide structured bug report template with auto-capture (device model, iOS version, style file hash)
- R-QA-03: Domain expert unavailable during critical sprint → pre-schedule A/B sessions
- R-QA-04: Beta musicians crash during real show → Crashlytics auto-capture + hotfix within 24h

**Acceptance Criteria:**
- AC-QA-01: Internal QA passes: all P0/P1 criteria green on reference devices
- AC-QA-02: Beta cohort: ≥ 18/20 musicians complete ≥ 3 shows each
- AC-QA-03: Exit beta: ≥ 18/20 report "no crash in 2h show"
- AC-QA-04: Exit beta: 0 reports of "section switch off-beat" in final 2 weeks
- AC-QA-05: A/B musicality score ≥ 3.5/5 on final NTT table version
- AC-QA-06: 500-style corpus import: ≥ 95% SFF1, ≥ 75% SFF2

---

## WS-12: Commercial Packaging

**Objective:** Prepare the product for App Store launch — pricing, metadata, marketing assets, and distribution infrastructure. This workstream activates at Gate 9 and completes at Gate 10. All copy and assets MUST comply with ADR-013 terminology policy.

**Scope:**
- **Commercial Moat Definition (per ADR-013):**
  - Moat = arranger workflow, live UX, import compatibility, timing quality, reliability, cloud ecosystem, AI-assisted workflow, musician productivity
  - NOT = bundled sounds, hardware ROM emulation, manufacturer content distribution
  - All marketing copy must reflect the true moat — never claim sound quality or bundled content as a reason to buy

- **Pricing Model:**
  - Freemium: free download, import 5 styles only
  - Pro Unlock: one-time $29.99 (validate with beta: $29.99 vs $39.99)
  - Phase 2: subscription tier $9.99/month or $79.99/year (Pro Plus features)
  - Phase 3: marketplace commission (30%) on creator style/sound pack sales
  - No introductory discount (app is already low-priced vs. $2,000-4,000 hardware)

- **App Store Metadata:**
  - App name (trademark-cleared)
  - Subtitle: "Live Arranger Keyboard Player"
  - Description: features, import compatibility, limitations (clear SFF2 support level). MUST include: "For best sound, connect your MIDI keyboard or sound module — this app is an arranger, not a sound module."
  - Keywords: arranger, keyboard, MIDI, style player, live performance, band, SoundFont
  - Screenshots: 5 portrait + 5 landscape (showing Live Screen, Mixer, Browser, Import, Setlist)
  - App Preview video: 30-second demo of live use
  - Privacy Policy URL + Support URL + Marketing URL
  - ⛔ Forbidden keywords: "Genos sounds", "PSR sound pack", "Tyros voices", "Yamaha SoundFont"

- **Marketing Assets:**
  - Demo video: "From style file to live performance in 60 seconds" (vendor-neutral)
  - YouTube channel: tutorials, tips, feature showcases
  - Facebook group presence: participate in nhạc công groups (organic, not paid)
  - Forum presence: keyboard community forums, Reddit r/WeAreTheMusicMakers
  - Word-of-mouth referral: primary growth channel for MVP
  - ⛔ No "Genos sounds included" / "PSR sound pack" / "Tyros voices" in any marketing copy
  - All marketing copy must be reviewed against ADR-013 terminology compliance checklist

- **Distribution:**
  - App Store (iOS/iPadOS 16+): primary channel
  - TestFlight: beta distribution only
  - No other distribution channels (MVP)

- **Support Infrastructure:**
  - In-app feedback/report button
  - Email support (support@appname.com)
  - FAQ page on website (knowledge base)
  - Known limitations document (public, in English + Vietnamese)

- **Analytics (Opt-in, Post-Privacy Policy):**
  - Optional crash reporting (Crashlytics)
  - Optional usage metrics (which features used, import success rate, device mix)

- **Localization:**
  - MVP: English (primary), Vietnamese (secondary)
  - UI strings via .stringsdict + SwiftUI localization
  - Error messages in both English and Vietnamese
  - Phase 2: Japanese, Spanish, Portuguese, Filipino

**Out of Scope:**
- Paid advertising (budget-constrained; rely on organic + word-of-mouth)
- PR agency (founder does press outreach)
- Influencer marketing (Phase 2 — after product-market fit)
- Enterprise licensing (Phase 3)

**Owner Role:** Commercial/Product Owner (Founder)

**Dependencies:**
- Depends on: WS-09 (trademark-clear app name, App Store compliance)
- Depends on: WS-10 (App Store Connect setup, build pipeline)
- Provides to: All workstreams — launch date commitment

**Deliverables:**
- D-WS12-01: Pricing model final (validated by beta)
- D-WS12-02: App Store metadata package (name, subtitle, description, keywords, screenshots, preview video) — reviewed for ADR-013 terminology compliance
- D-WS12-03: Privacy Policy URL (with user-uploaded sound consent clause)
- D-WS12-04: Support infrastructure (email, FAQ page)
- D-WS12-05: Demo video (60-90 seconds) — vendor-neutral language
- D-WS12-06: YouTube channel with 3 tutorial videos — all reviewed for terminology compliance
- D-WS12-07: Known limitations document (public, in English + Vietnamese) — includes sound source limitations and MegaVoice handling guide
- D-WS12-08: Marketing launch plan (forum posts, community campaign, email blast) — pre-reviewed for ADR-013 compliance
- D-WS12-09: Pre-launch checklist (all items must be green, including ADR-013 terminology audit)
- D-WS12-10: Launch post-mortem template
- D-WS12-11: Commercial moat positioning document (per ADR-013 — what we compete on and what we do NOT compete on)
- D-WS12-12: External sound setup guide for musicians (how to connect MIDI keyboard/module for best fidelity)

**Risks:**
- R-COM-01: App Store rejection → plan 2-week buffer in schedule for resubmission
- R-COM-02: Pricing too high vs. musician WTP → A/B test with beta; prepare $19.99 fallback
- R-COM-03: Low organic discovery → invest in YouTube tutorial content pre-launch
- R-COM-04: Negative reviews due to misaligned expectations (SFF2 limitations, GM sound quality) → set clear expectations in description: "Connect your keyboard for best sound"
- R-COM-05: Competitor marketing claims bundled sounds → remain focused on arranger quality moat, do not engage in sound quality comparison wars

**Acceptance Criteria:**
- AC-COM-01: App name trademark-clear (US + EU + JP)
- AC-COM-02: Pricing model validated by ≥ 10 beta musicians
- AC-COM-03: App Store metadata complete and submitted — zero instances of forbidden terminology
- AC-COM-04: Privacy Policy published at accessible URL
- AC-COM-05: Demo video published on YouTube — vendor-neutral language confirmed
- AC-COM-06: Launch day: App Store approved, version live, support channels active
- AC-COM-07: All commercial copy audited against ADR-013 terminology checklist — zero violations
- AC-COM-08: External sound setup guide published (English + Vietnamese)

---

# C. GATE-BASED DELIVERY PLAN (GATE 0 → GATE 10)

---

## GATE 0 — Project Sanity & Scope Lock

**Objective:** Lock the vision, team, architecture, and MVP scope before any production code is written. De-risk all major unknowns. If this gate fails, the project does not proceed.

**Entry Criteria (must be met before gate opens):**
- ✅ All 19 spec documents written and reviewed
- ✅ Team: Senior C++ Audio Engineer identified (at minimum)
- ✅ Team: iOS/SwiftUI Engineer identified (at minimum)
- ✅ Team: Domain Expert / Musician committed (part-time)
- ✅ JUCE Indie license purchased
- ✅ Apple Developer Program enrollment confirmed
- ✅ 10 strategic decisions (from live-arranger-strategy.md) documented and signed off

**Exit Criteria (gate is passed when ALL are complete):**
- G0-01: 500+ style test corpus collected (prototype SFF1/SFF2, full variety)
- G0-02: SFF1 hex dump × 5 files + CASM structure verified against JJazzLab cross-ref
- G0-03: Sprint 0 Spike M0 complete: parse 1 .sty → create UASF → play Main A via CoreMIDI output
- G0-04: Sprint 0 Audio Spike complete: JUCE loop + sfizz noteOn → sound out, latency measured
- G0-05: UASF Spec v1.0-rc approved by team
- G0-06: Technical Architecture v1.0 approved (including all ADRs)
- G0-07: MVP scope fully locked (no feature additions without re-opening gate)
- G0-08: CI pipeline running (build + unit test + corpus import test)
- G0-09: Risk Register v1.0 with all items assessed and mitigation actions assigned
- G0-10: Open Questions (22 from execution-bundle.md) documented with investigation assignments

**Deliverables:**
- G0-D1: SFF1 Ctab field offset documentation (validated by hex dump + corpus)
- G0-D2: JUCE project skeleton (audio callback, MIDI out, lock-free queue utilities)
- G0-D3: CI pipeline configuration
- G0-D4: Open Questions tracker (status: OPEN / INVESTIGATING / DECIDED / CLOSED)
- G0-D5: Risk Register v1.0

**Tests Required:**
- T-G0-01: Spike M0 must pass: SFF1→UASF→CoreMIDI out, 5 styles, no crash, timing audible-correct
- T-G0-02: Spike Audio must pass: JUCE loop + sfizz 1 note, latency < 10ms (relaxed), no glitch 5 min
- T-G0-03: CI build + test suite must complete in < 10 min

**Evidence Required:**
- E-G0-01: Spike M0 screen recording (MIDI monitor showing correct notes on beat)
- E-G0-02: Spike Audio latency measurement (oscilloscope or audio editor screenshot)
- E-G0-03: CI pipeline green build screenshot
- E-G0-04: All 22 open questions documented with assigned owners

**Accountable (RACI):**
- Responsible: Tech Lead (runs spike, documents findings)
- Accountable: Product Manager (signs off that risks are acceptable)
- Consulted: Audio/MIDI Engineer (implements spike), Domain Expert (musical validation)
- Informed: All team members

**Risks:**
- R-G0-01: Spike M0 fails → CASM parse fundamentally wrong → pivot: ship as MIDI-style-only player (no chord transform)
- R-G0-02: Spike Audio fails → JUCE+sfizz integration too complex for timeline → pivot: MIDI-only MVP
- R-G0-03: Team not assembled → halt project until key roles filled

**Kill Criteria (gate fails, decision required):**
- KC-G0-01: Spike M0 not achieved in 3 weeks → project risk too high
- KC-G0-02: No viable C++ audio engineer found in 6 weeks of recruiting → project cannot proceed
- KC-G0-03: Test corpus reveals CASM structure fundamentally incompatible with current approach → major architecture pivot required

**Decision: [ ] Go / [ ] No-Go / [ ] Pivot**

---

## GATE 1 — Realtime Timing Spike (De-Risk the Jitter Problem)

**Objective:** Prove that the audio-callback-driven musical clock produces sub-millisecond timing accuracy on real iPad hardware. This is the highest-risk technical challenge — if timing jitter is unacceptable, the architecture must change.

**Entry Criteria:**
- ✅ G0 completed successfully (all exit criteria met)
- ✅ MusicalClock C++ implementation ready (double-precision tick counter, sample-derived)
- ✅ Event scheduling code for section playback (render events in [blockStartTick, blockEndTick))
- ✅ CoreMIDI output with MIDITimeStamp scheduling
- ✅ Test harness for timing measurement (offline render + MIDI file parse + tick analysis)
- ✅ Reference iPad device (iPad Air M1 minimum)

**Exit Criteria:**
- G1-01: MusicalClock drift measured: < 1 frame error after 30 minutes continuous playback
- G1-02: Section boundary timing: P99 event placement error < 2 tick at 480 PPQ (≈ 2ms @ 120 BPM)
- G1-03: CoreMIDI output: events delivered at correct frame positions (verified by MIDI loopback hardware)
- G1-04: Lock-free commandQueue: no blocking, no dropped commands in 1h stress test
- G1-05: Chord change → retrigger events emitted at correct tick (no 1-buffer delay)
- G1-06: Published EngineState atomic snapshot readable from UI thread at 60Hz without blocking audio

**Deliverables:**
- G1-D1: MusicalClock unit test results (drift measurement, precision report)
- G1-D2: Timing stress test report (30 min offline render, event timing distribution histogram)
- G1-D3: CoreMIDI output timing validation (hardware loopback: 1,000 trials)
- G1-D4: Lock-free queue benchmarks (throughput, blocking probability)
- G1-D5: Architecture Decision Record: "Audio-Clock-Driven Scheduling v1.0" (confirm or document lessons)

**Tests Required:**
- T-G1-01: MusicalClock drift: render 30 min silence with periodic tick check
- T-G1-02: Event timing: render Main A 1,000 bars, measure every event position vs. ideal tick
- T-G1-03: CommandQueue: push 10,000 Start/Stop commands in 1 second, verify all processed in order
- T-G1-04: State snapshot: verify atomic reads from UI thread while audio thread writing at 256 buffer rate

**Evidence Required:**
- E-G1-01: Timing distribution histogram (event error in ticks, PDF/PNG)
- E-G1-02: Hardware MIDI loopback data (CSV of send vs. receive timestamps)
- E-G1-03: CPU profiler screenshot showing audio callback consistently < 40% duration

**RACI:**
- R: Audio/MIDI Engineer (implements and tests)
- A: Tech Lead (approves timing is acceptable)
- C: QA/Timing Engineer (validates test methodology)
- I: Product Manager, iOS Engineer

**Risks:**
- R-G1-01: CoreMIDI MIDITimeStamp scheduling not sample-accurate on iPad → alternative: use frame offset in shared memory buffer
- R-G1-02: Timer fusion on iOS causes occasional callback delay > buffer duration → monitor via CPU load, auto-increase buffer as fallback
- R-G1-03: Atomic snapshot too large for non-blocking read (cache-line bouncing) → split into cache-line-aligned substructures

**Kill Criteria:**
- KC-G1-01: P99 timing error > 5 ticks (≈ 5ms) at 256 buffer → architecture fundamentally flawed
- KC-G1-02: Cannot achieve < 2 tick error on iPad Air M1 → project cannot meet core requirement

**Decision: [ ] Go / [ ] No-Go / [ ] Pivot**

---

## GATE 2 — Minimal Arranger Vertical Slice

**Objective:** Build the complete "play a style with chord input" pipeline — import → load → chord → transform → MIDI out. This is the first end-to-end demo that proves the product concept works.

**Entry Criteria:**
- ✅ G1 completed (timing proven)
- ✅ SFF1 basic parser working (Main A section extraction from corpus)
- ✅ UASF builder working (manifest.json + section .mid + zip)
- ✅ Chord Engine lookup table generated (14 MVP types, fingered mode)
- ✅ ArrangerEngine: render loop, NTR (rootTransposition), NTT (bass), retrigger (stop, pitchShift, pitchShiftToRoot)
- ✅ CoreMIDI input (chord receive) working

**Exit Criteria:**
- G2-01: Full pipeline operational: connect MIDI keyboard → play chord → style plays with correct transform via MIDI out
- G2-02: NTR rootTransposition verified: transpose style from CMaj to any key, bass and chord tracks follow correctly
- G2-03: NTT bass mode verified: chord change retriggers bass to new root
- G2-04: Main Section A loop: plays continuously, correct event sequence each bar
- G2-05: Section switch (no fill, bar-boundary): Main A → Main B, commit at correct bar boundary
- G2-06: Panic: all notes off, transport continues
- G2-07: 5 different style files from corpus: all play correctly (musician A/B test passes at ≥ 3/5)
- G2-08: Chord Engine: fingered mode recognizes 14 MVP chord types correctly (unit tests 100% pass)
- G2-09: **External MIDI playback validation:** MIDI output verified through USB MIDI device (loopback test), events arrive at correct sample positions

**Deliverables:**
- G2-D1: End-to-end demo video (keyboard → app → MIDI sound module)
- G2-D2: Vertical slice integration test (automated, CI-passing)
- G2-D3: NTR/NTT transform unit tests (1,440 + 2,448 cases)
- G2-D4: Chord Engine unit tests (900+ cases, 100% pass)
- G2-D5: A/B test results (5 styles, comparison with Yamaha reference, domain expert score)
- G2-D6: External MIDI playback timing validation (USB loopback: 1,000 events, verify correct timing)

**Tests Required:**
- T-G2-01: Full pipeline: 5 style × 4 chords × 4 bar = 20 test scenarios, all pass
- T-G2-02: NTR: transposition from C → every other key (11 × 4 tracks = 44 scenarios)
- T-G2-03: NTT bass: Cmaj7 → Cm → C7 → Cdim — each chord change triggers correct retrigger
- T-G2-04: Section switch: Main A → Main B at bar 4, verify event alignment
- T-G2-05: Panic + restart: play → panic → verify 0 active notes → restart → play normally

**Evidence Required:**
- E-G2-01: Screen recording: chord input → style output with section switch
- E-G2-02: Unit test pass log (all ≥ 4,000 cases green)
- E-G2-03: A/B test score sheet (5 styles × domain expert rating)
- E-G2-04: MIDI monitor output showing correct transposition on chord change

**RACI:**
- R: Audio/MIDI Engineer (implement + test)
- A: Tech Lead (signs off on musical quality)
- C: Domain Expert (A/B test scoring)
- I: Product Manager, iOS Engineer, QA

**Risks:**
- R-G2-01: NTT music theory baseline sounds wrong to musician ear → acceptable for MVP; refine in beta
- R-G2-02: Chord Engine ambiguous on 2-note inputs → favor simpler chord (Major > 7 > etc.)
- R-G2-03: Style import fails on complex SFF1 files → target 90%+ for simple files; complex files get manual investigation

**Kill Criteria:**
- KC-G2-01: Cannot play a single style correctly through the full pipeline → fundamental architecture gap
- KC-G2-02: NTT transform produces musically unacceptable output on > 50% of chords → NTT algorithm fundamentally wrong
- KC-G2-03: Chord Engine fails on > 20% of common chord inputs → recognition algorithm needs redesign

**Decision: [ ] Go / [ ] No-Go / [ ] Pivot**

---

## GATE 3 — Internal Style Format v1

**Objective:** Lock the UASF format schema, implement the reader/writer/validator, migrate all existing test styles to UASF, and prove round-trip stability.

**Entry Criteria:**
- ✅ G2 completed (vertical slice works)
- ✅ UASF Spec v1.0-rc reviewed by team
- ✅ All SFF1 bit-level offsets validated (Sprint 0 spike)
- ✅ 200+ styles imported to UASF for testing

**Exit Criteria:**
- G3-01: UASF Spec v1.0 finalized and signed off by Tech Lead (including articulation metadata extension per ADR-013)
- G3-02: UASF reader/writer C++ library: read .uas → UASFStyle struct → write .uas → binary identical (except timestamps)
- G3-03: Validation engine (V01-V07, W01-W06) correctly rejects all invalid scenario fixtures
- G3-04: All 200+ test styles round-trip: import SFF1 → UASF → parse → re-serialize → re-parse → identical output
- G3-05: UASF migration framework: v0.9 → v1.0 migration works correctly (test fixture)
- G3-06: Schema JSON file (manifest.schema.json) published — including articulation metadata and fidelity flag fields
- G3-07: 34-value chord type enum finalized and shared with Chord Engine and Import Adapter
- G3-08: Yamaha→UASF mapping table documented (Ctab fields → TrackRule, Sdec → section metadata)
- G3-09: **Articulation metadata schema reviewed and approved:** articulation_profile, fidelity_requirement, recommended_playback, articulation_hints all defined with example values
- G3-10: **Fidelity flags documented:** clear semantics for low/medium/high fidelity_requirement and how runtime engine resolves rendering mode

**Deliverables:**
- G3-D1: UASF Spec v1.0 document (final, with all sections complete, articulation metadata extension per ADR-013)
- G3-D2: manifest.schema.json (with articulation metadata fields)
- G3-D3: UASF C++ reader/writer library (with unit test suite)
- G3-D4: Validation engine (V01-V07, W01-W06 error fixtures + test cases)
- G3-D5: Migration framework + v0.9→v1.0 test
- G3-D6: Chord type enum header file (shared across modules)
- G3-D7: Yamaha→UASF mapping table documentation
- G3-D8: Articulation metadata specification document (ADR-013 articulation section with examples)
- G3-D9: Fidelity flag and rendering mode resolution documentation

**Tests Required:**
- T-G3-01: Round-trip test: SFF1 import → UASF → parse → edit manifest → write → re-parse → no data loss
- T-G3-02: Validation: all V01-V07 + W01-W06 errors/warnings produce correct code; no false positives on valid files
- T-G3-03: Migration: v0.9 file → engine v1.0 → migrated UASF → verified correct
- T-G3-04: Corpus: 200+ styles imported, all validated, import limitation counts tracked
- T-G3-05: Serialization: ensure binary MIDI data in sections/*.mid is byte-identical after round-trip
- T-G3-06: Articulation metadata round-trip: populate articulation_profile → serialize → parse → identical profile

**Evidence Required:**
- E-G3-01: Round-trip test log (200 styles, success rate ≥ 99%)
- E-G3-02: Validation test output (7 error types, 6 warning types — all triggered correctly)
- E-G3-03: Schema JSON file (validated against UASF spec §4, including articulation fields)
- E-G3-04: Corpus import report (import rates, limitation distribution chart)
- E-G3-05: Articulation metadata test fixtures showing correct profile assignment

**RACI:**
- R: Tech Lead (schema ownership) + Audio/MIDI Engineer (reader/writer implementation)
- A: Tech Lead (spec sign-off)
- C: All engineers (schema review, articulation metadata review)
- I: Product Manager, QA

**Risks:**
- R-G3-01: Schema changes needed after corpus testing > 500 styles → anticipate v1.1 within first beta cycle
- R-G3-02: MIDI tick normalization rounding errors compound → use double-precision tick internally, round only at output
- R-G3-03: Articulation metadata schema incomplete for Genos-specific articulation patterns → extension mechanism allows future profile types without breaking existing files

**Kill Criteria:**
- KC-G3-01: Round-trip corruption detected (data loss on serialize→deserialize) → format design flaw
- KC-G3-02: Schema cannot represent critical CASM fields discovered from corpus testing
- KC-G3-03: UASF file size unexpectedly large (> 2MB average) → reevaluate container strategy

**Decision: [ ] Go / [ ] No-Go / [ ] Pivot**

---

## GATE 4 — Yamaha SFF1 Importer Spike

**Objective:** Achieve ≥ 95% SFF1 import success rate on the test corpus. Build production-grade parser with all limitation tracking.

**Entry Criteria:**
- ✅ G3 completed (UASF format locked)
- ✅ SFF1 parser prototype from Sprint 0 working (basic section extraction + CASM)
- ✅ Test corpus expanded to 500+ files
- ✅ JJazzLab YamJJazz cross-reference completed

**Exit Criteria:**
- G4-01: SFF1 import success rate ≥ 95% on 500-style corpus
- G4-02: SFF2 basic import success rate ≥ 75% (MIDI sections + basic CASM; advanced features degraded gracefully)
- G4-03: All limitation codes (15 from file-import-strategy.md) implemented with human-readable messages
- G4-04: Import throughput: average < 2s per style (iPad Air M1)
- G4-05: Drum map: all Yamaha drum notes mapped to GM equivalents (overrides for non-GM notes)
- G4-06: Sound map builder: Yamaha voice → GM program lookup table complete
- G4-07: Multi-range Ctb2 (SFF2): parsed correctly, UASF noteRanges populated (may use first range only)
- G4-08: Batch import: 20 files in parallel → each returns correct ImportResult, total < 40s
- G4-09: Zero crashes on any file in corpus (graceful rejection with error message for unsupported)
- G4-10: **MegaVoice detection matrix validated:** MegaVoice tracks detected using velocity range + note range + NTR pattern heuristics, no false positives on standard tracks
- G4-11: **Unsupported articulation handling:** Tracks with unrecognised articulation patterns produce SFF2_UNSUPPORTED_ARTICULATION limitation with clear user message, preserved as standard MIDI

**Deliverables:**
- G4-D1: SFF1 production parser (full section detection, CASM parse, all limitation codes)
- G4-D2: SFF2 production parser (Ctb2 multi-range, extended NTT enum, MegaVoice detection + articulation metadata population)
- G4-D3: Drum note mapper (Yamaha→GM, with override table)
- G4-D4: Sound map builder (Yamaha voice→GM program lookup)
- G4-D5: Import pipeline documentation (architecture, extension points for new formats, MegaVoice handling guide)
- G4-D6: Corpus test automation (runs nightly, tracks success rates by model and genre)
- G4-D7: Regression fixture for every new bug discovered during corpus testing
- G4-D8: MegaVoice detection matrix specification (thresholds, patterns, test fixtures)
- G4-D9: Unsupported articulation handling test suite (10+ edge case files)

**Tests Required:**
- T-G4-01: Full corpus import: 500 files, measure success rate, warning distribution, timing
- T-G4-02: Per-model validation: group corpus by Yamaha model, report success rate per model
- T-G4-03: Per-genre validation: group by genre, check for genre-specific issues
- T-G4-04: Healing test: file that failed on previous version → retest after fix
- T-G4-05: Malformed file test: 10 corrupted files → all rejected gracefully, no crash
- T-G4-06: Sound map accuracy: verify GM program numbers against known Yamaha voice mappings
- T-G4-07: MegaVoice detection test: known MegaVoice files from corpus → all detected correctly with articulation_profile="mega_like"
- T-G4-08: Unsupported articulation test: edge case files → produce SFF2_UNSUPPORTED_ARTICULATION without crash or silent degradation

**Evidence Required:**
- E-G4-01: Corpus import report (success rate ≥ 95% SFF1, ≥ 75% SFF2)
- E-G4-02: Limitation distribution (top 5 codes, % of files affected)
- E-G4-03: Per-model breakdown (which models have lowest success rate)
- E-G4-04: Import timing histogram (P50, P95, max)
- E-G4-05: Zero-crash confirmation log for all 500+ files
- E-G4-06: MegaVoice detection results: which files flagged, accuracy vs. known ground truth
- E-G4-07: Unsupported articulation handling results: all edge cases handled gracefully

**RACI:**
- R: Audio/MIDI Engineer (parser implementation)
- A: Tech Lead (accuracy approval)
- C: QA/Timing Engineer (corpus test automation)
- I: Product Manager, iOS Engineer

**Risks:**
- R-G4-01: SFF2 import rate < 60% → lower target to 60%, publish clear limitation list
- R-G4-02: Genos/Tyros styles use features not documented in any open-source reference → specific model research
- R-G4-03: Some style files in corpus may be corrupted or partial → filter corpus during validation
- R-G4-04: MegaVoice detection thresholds cause false positives/negatives → prefer false negatives (standard playback) over false positives (wrongly restricted playback); tune in corpus validation

**Kill Criteria:**
- KC-G4-01: SFF1 import success < 80% → parser fundamentally missing key CASM features
- KC-G4-02: Import causes crash on > 1% of files → memory safety issue in parser
- KC-G4-03: Sound map produces wrong GM programs > 10% of the time → lookup table fundamentally wrong

**Decision: [ ] Go / [ ] No-Go / [ ] Pivot**

---

## GATE 5 — Live iPad UX Prototype

**Objective:** Deliver a functional Live Screen UI that connects to the engine — section pads, chord display, transport, panic. The UI must be testable by beta musicians.

**Entry Criteria:**
- ✅ G2 completed (engine pipeline exists)
- ✅ EngineState struct finalized (atomic snapshot fields)
- ✅ Command objects finalized (all Start/Stop/Section/Tempo/Panic commands)
- ✅ Swift/C++ interop bridge working (command enqueue + state snapshot read)

**Exit Criteria:**
- G5-01: Live Screen UI: section pad grid renders with all states (default/active/pending/disabled)
- G5-02: Chord display: shows chord name from EngineState, updates within 1 frame of chord change
- G5-03: Transport: Start/Stop buttons send commands to engine, state reflected correctly
- G5-04: Panic button: 88×88pt, always visible, sends Panic command, no confirm dialog
- G5-05: Performance mode: 3-finger toggle, hides tab bar, all live controls remain active
- G5-06: Tempo control: tap-to-edit works, tap tempo with 4-tap average
- G5-07: EngineState polling: UI updates at 60Hz, no frame drops on iPad Air M1
- G5-08: Basic Mixer: per-track volume sliders with atomic write to engine
- G5-09: Section pending indicator: amber pulsing at 0.8Hz when section queued
- G5-10: Portrait mode: layout adjusts gracefully (Main A-D + Fill always visible, scroll for Intro/Ending)

**Deliverables:**
- G5-D1: Live Screen SwiftUI component (section pads, chord display, transport, panic)
- G5-D2: ArrangerBridge Swift/C++ interop layer
- G5-D3: Mixer panel (collapsible, per-track controls)
- G5-D4: Performance mode implementation (gesture lock)
- G5-D5: Command types (Swift value types → C++ Command struct)
- G5-D6: UI responsiveness benchmark (high-speed camera, P95 ≤ 100ms tap→visual)

**Tests Required:**
- T-G5-01: Section pad tap → EngineState reflects new pending → pad amber → boundary commit → pad cyan
- T-G5-02: Chord change → chord display updates within 1 frame (16.7ms)
- T-G5-03: Panic button → engine stops all notes within 50ms
- T-G5-04: Performance mode toggle: 3-finger tap → lock → no navigation possible
- T-G5-05: State polling: 60Hz poll from Swift, no observable main-thread stalls
- T-G5-06: Mixer slider → engine track volume changes within 2 frames

**Evidence Required:**
- E-G5-01: High-speed camera recording: section pad tap → visual feedback → engine commit timing
- E-G5-02: Screen recording: full live session (section switching, chord changes, panic)
- E-G5-03: Instruments trace: main thread CPU < 30%, no frame drops
- E-G5-04: A/B comparison: section pad interaction vs. Yamaha hardware (subjective)

**RACI:**
- R: iOS/SwiftUI Engineer (UI implementation)
- R: Audio/MIDI Engineer (bridge layer)
- A: Product Manager (UX sign-off)
- C: Designer (visual polish)
- I: QA, Tech Lead

**Risks:**
- R-UX-01: SwiftUI performance with rapid state updates → use diffable state, throttle to 60Hz display link
- R-UX-02: Cross-fade animation on chord display causes layout shift → fixed-size text frame
- R-UX-03: Performance mode gesture conflicts with other iOS gestures → test on iPad with various cases

**Kill Criteria:**
- KC-G5-01: UI responsiveness > 200ms on any tap → SwiftUI + engine bridge architecture needs redesign
- KC-G5-02: Performance mode leaks navigation → security finder gesture bypasses lock

**Decision: [ ] Go / [ ] No-Go / [ ] Pivot**

---

## GATE 6 — Hardware Compatibility Lab

**Objective:** Validate the app works with real MIDI hardware in real-world conditions. Test with minimum 10 different MIDI USB keyboards, modules, and interfaces.

**Entry Criteria:**
- ✅ WS-02 MIDI engine fully implemented
- ✅ WS-07 device routing system implemented
- ✅ 5+ different MIDI devices available for testing
- ✅ iConnectivity mio or equivalent MIDI loopback hardware available

**Exit Criteria:**
- G6-01: 10+ MIDI devices tested: plug-and-play with all class-compliant USB MIDI devices
- G6-02: BLE MIDI: connects and works (latency warning displayed)
- G6-03: Virtual MIDI port: visible to AUM and GarageBand on same iPad
- G6-04: Hot-plug: device disconnect → transport continues (no crash, no stop) → banner shown → reconnect → init sequence sent → playback resumes
- G6-05: Multi-device routing: track 1 → device A, track 2 → device B, track 3 → internal, simultaneously
- G6-06: Init sequence per device (GM/GS/XG selectable): correct voice sound on all tested modules
- G6-07: Sustain pedal (CC64): hold mode works correctly
- G6-08: 2h continuous session: no MIDI drift, no orphan notes, no device dropout
- G6-09: Power cycle test: unplug/replug iPad USB hub → app recovers without restart
- G6-10: Audio+MIDI compatibility: no audible glitch when MIDI and audio streams simultaneously
- G6-11: **External Yamaha device validation:** At least 1 Yamaha keyboard/module tested for full playback including correct voice selection, MIDI channel routing, articulation pass-through
- G6-12: **Articulation pass-through test:** MegaVoice track with articulation_profile="mega_like" → routed to external device → correct velocity layers and note ranges preserved on device output

**Deliverables:**
- G6-D1: Hardware compatibility test report (10+ devices, per-device test results, including Yamaha-specific articulation pass-through)
- G6-D2: Known device issues document (model-specific quirks with workarounds)
- G6-D3: MIDI setup guide for musicians (how to connect, common issues, troubleshooting) — includes external Yamaha device section for best MegaVoice playback
- G6-D4: MIDI clock out validation (jitter measurement)
- G6-D5: External Yamaha device articulation pass-through test report

**Tests Required:**
- T-G6-01: Plug-and-play: connect 10 different USB MIDI devices, verify each is recognized within 1s
- T-G6-02: Hot-plug: disconnect while playing → verify 3 behaviors (no crash, banner, transport continues)
- T-G6-03: Multi-device: route 3 tracks to 3 different outputs, play 5 minutes, verify correct routing
- T-G6-04: Init sequence: load style → device receives correct PC/Bank/CC per track
- T-G6-05: 2h continuous: play through setlist of 20 songs, log any MIDI errors
- T-G6-06: Yamaha external playback: style with MegaVoice tracks → route to Yamaha device → verify articulation layers correctly triggered
- T-G6-07: Articulation pass-through: external device receives correct velocity values for MegaVoice layers, no clipping or flattening of articulation data

**Evidence Required:**
- E-G6-01: Device test matrix (10+ devices × 10 test scenarios each → 100+ pass/fail cells)
- E-G6-02: MIDI monitor capture: correct init sequence, correct notes, correct routing
- E-G6-03: 2h stability log: no MIDI errors, no orphan notes, no device dropout
- E-G6-04: Video: hot-plug test (disconnect while playing → no crash → reconnect → resume)
- E-G6-05: Yamaha device test: MIDI monitor showing correct articulation velocity layers on MegaVoice tracks

**RACI:**
- R: Audio/MIDI Engineer (device test execution)
- A: QA/Timing Engineer (test methodology, result validation)
- C: Domain Expert (tests with real stage setup)
- I: Product Manager, iOS Engineer

**Risks:**
- R-G6-01: Specific MIDI device has unique quirk → log as known issue for v1.0
- R-G6-02: USB hub power delivery insufficient on older iPad → document recommended hardware
- R-G6-03: BLE MIDI reconnection slow on stage → recommend USB for live shows

**Kill Criteria:**
- KC-G6-01: > 3/10 devices require manual configuration to work → plug-and-play promise broken
- KC-G6-02: Hot-plug causes crash > 1/10 tests → MIDI device management has fundamental flaw

**Decision: [ ] Go / [ ] No-Go / [ ] Pivot**

---

## GATE 7 — Musician Alpha

**Objective:** First internal usability validation. The app is complete enough for a musician to:
(a) import their own styles, (b) play a live set with section switching, (c) survive a 2-hour show without crashing.

**Entry Criteria:**
- ✅ G2 (engine pipeline) + G4 (importer) + G5 (UI prototype) + G6 (hardware) complete
- ✅ 500-style corpus import: ≥ 95% SFF1, ≥ 75% SFF2
- ✅ Mixer panel functional (volume/pan/mute/solo/routing)
- ✅ Setlist basic functionality (create, add songs, assign style, reorder)
- ✅ Internal sound "good enough" (sfizz + GM bank, single layer)

**Exit Criteria:**
- G7-01: Internal alpha test: 3 internal testers (including domain expert) each complete 2h session
- G7-02: Zero crashes in all 3 internal test sessions (6 hours total)
- G7-03: Zero section switch timing complaints from domain expert
- G7-04: Import flow: domain expert imports 50 personal styles, success rate ≥ 90%
- G7-05: Chord recognition: domain expert rates accuracy ≥ 4/5 for fingered mode
- G7-06: Internal sound: domain expert rates quality ≥ 3/5 ("usable for practice, Phase 2 for stage")
- G7-07: Mixer: volume/pan/mute/solo changes take effect within 2 frames
- G7-08: Setlist: create 20-song setlist in < 5 minutes
- G7-09: A/B test: app vs. Yamaha Genos, same style, ≥ 3.5/5 similarity score
- G7-10: All P0/P1 bugs triaged: P0 fixed, P1 fix-planned
- G7-11: **Musician evaluation of degraded vs. external playback:** Domain expert compares same style in (a) external Yamaha device playback vs. (b) GM fallback mode — scores each for musicality, provides qualitative feedback on acceptability
- G7-12: MegaVoice handling validated: domain expert confirms MegaVoice tracks are NOT silently muted or GM-silenced, and that the fallback warning is clear and appropriate

**Deliverables:**
- G7-D1: Alpha test report (detailed per-session log, includes sound source comparison data)
- G7-D2: P0 bug fix changelog
- G7-D3: Known issues list for beta (must-read before using) — includes MegaVoice handling guide and sound setup recommendations
- G7-D4: User-facing documentation (quick start guide — 1 page) — includes "for best sound, connect your MIDI keyboard/module" guidance
- G7-D5: Alpha exit decision (Go/No-Go for beta)
- G7-D6: Sound source comparison report: degraded vs. external playback scoring and recommendations

**Tests Required:**
- T-G7-01: 2h stability: play continuously, switch style 10 times, switch section 50 times, panic 5 times
- T-G7-02: Import stability: import 100 style files, stress app with background import while playing
- T-G7-03: Setlist flow: create → play → next song preload → play → all correct
- T-G7-04: Memory after 2h: verify < 10% growth from baseline
- T-G7-05: Domain expert A/B test: 10 styles, compare chord transform accuracy
- T-G7-06: Sound quality comparison: same style played through external device vs. GM fallback, domain expert rates each 1-5, documents the acceptability gap

**Evidence Required:**
- E-G7-01: 6-hours crash-free log (3 × 2h sessions)
- E-G7-02: Domain expert A/B scoring sheet (10 styles, similarity 1-5)
- E-G7-03: Sound source comparison scoring (3 styles × 2 modes = 6 ratings, plus qualitative feedback)
- E-G7-03: Corpus import report (final, with domain expert's personal styles)
- E-G7-04: Memory usage graph (2h session, Instruments allocation track)
- E-G7-05: Known issues document (reviewed by domain expert for clarity)

**RACI:**
- R: QA/Timing Engineer (alpha test execution)
- A: Product Manager (sign-off to proceed to beta)
- C: Domain Expert (musical validation)
- C: Audio/MIDI Engineer + iOS Engineer (bug fixes)
- I: All team

**Risks:**
- R-ALPHA-01: Domain expert unavailable for prolonged testing → schedule sessions well in advance
- R-ALPHA-02: Internal sound quality too poor for musician to take seriously → emphasize "MIDI-out first" during alpha
- R-ALPHA-03: P0 bugs found late in alpha → accept beta delay or ship with known critical bugs documented

**Kill Criteria:**
- KC-G7-01: Any crash in any alpha session → beta must not proceed until root cause found and fixed
- KC-G7-02: Domain expert rates overall experience < 3/5 → product not ready for beta
- KC-G7-03: Import success rate on domain expert's styles < 80% → importer needs more work

**Decision: [ ] Go / [ ] No-Go / [ ] Pivot**

---

## GATE 8 — Stability Hardening

**Objective:** Eliminate all crashes and critical timing issues found during alpha. Achieve 0-crash 2h stability on reference hardware.

**Entry Criteria:**
- ✅ G7 alpha completed — all alpha bugs documented
- ✅ Crashlytics integrated for crash reporting
- ✅ Memory stability test harness (`malloc` logging, diff)

**Exit Criteria:**
- G8-01: 3 consecutive 2h stability tests: 0 crashes, 0 audio dropouts > 100ms
- G8-02: Memory leak: usage after 2h session ≤ baseline × 1.1 (verified by Instruments)
- G8-03: Timing stress test (30 min offline): P99 < 2 tick, MAX < 5 tick, 0 orphan notes
- G8-04: All P0 bugs from alpha: fixed and verified (cannot reproduce)
- G8-05: All P1 bugs from alpha: fix completed or plan documented for beta
- G8-06: ASan (AddressSanitizer) nightly: 0 memory issues detected in 2h test
- G8-07: Thread Sanitizer (TSan): 0 data races in all unit + integration tests
- G8-08: 500-style corpus import: 100% pass (no import crash, even for rejected files)
- G8-09: MIDI orphan note test: 0 orphan notes after 30 min continuous with random chord changes
- G8-10: Performance regression: no CPU or memory metric regressed > 10% from Gate 7 baseline

**Deliverables:**
- G8-D1: Hardening test report (all test results, measured against exit criteria)
- G8-D2: ASan/TSan clean bill of health
- G8-D3: Performance baseline v1.0 (compare with all future releases)
- G8-D4: Known non-P0 issues list (for beta, with workarounds)

**Tests Required:**
- T-G8-01: 2h stability × 3: exact same scenario each run (automated script)
- T-G8-02: Memory ASan: 2h with ASan enabled (slower, but catches all memory errors)
- T-G8-03: Timing stress: 30 min with aggressive chord changes (every 2 beats), section switches (every 4 bars)
- T-G8-04: Orphan detection: parse full output MIDI — verify #NoteOn == #NoteOff per channel per pitch
- T-G8-05: TSan: run all integration tests with TSan on CI Mac

**Evidence Required:**
- E-G8-01: 3 × 2h crash-free console logs
- E-G8-02: ASan nightly report (7 consecutive nights, 0 issues)
- E-G8-03: TSan report (0 data races in integration tests)
- E-G8-04: Performance baseline numbers (CPU%, memory MB, timing P50/P95/P99/MAX)
- E-G8-05: Bug fix verification: list of all P0/P1 bugs → each linked to regression fixture

**RACI:**
- R: Audio/MIDI Engineer + iOS Engineer (fix bugs)
- R: QA/Timing Engineer (run tests, track metrics)
- A: Tech Lead (hardening complete)
- C: All engineers
- I: Product Manager

**Risks:**
- R-HARD-01: Memory leak found late → use heap profiling to find root cause within 48h
- R-HARD-02: Timing regression after fix → always run timing stress test with every bug fix PR
- R-HARD-03: ASan false positives in JUCE code → exclude JUCE from ASan, focus on our code

**Kill Criteria:**
- KC-G8-01: Any instability test fails after 3 fix attempts → engineering needs architecture-level review
- KC-G8-02: Cannot achieve 2h 0-crash → fundamental stability issue; must investigate before beta

**Decision: [ ] Go / [ ] No-Go / [ ] Pivot**

---

## GATE 9 — Beta Package

**Objective:** Ship the app to 20 external beta musicians for real-world validation. The app must be stable enough that musicians can use it in a real performance.

**Entry Criteria:**
- ✅ G8 hardening complete — 0 crash 2h × 3
- ✅ TestFlight beta configuration ready
- ✅ Crashlytics opt-in integrated
- ✅ All P0 bugs fixed
- ✅ Known issues document reviewed by domain expert
- ✅ Beta feedback protocol ready (form, community channel, SLA)

**Exit Criteria:**
- G9-01: TestFlight build published to 20 external testers
- G9-02: Beta onboarding email sent: instructions, known issues, feedback form, community link
- G9-03: First-week response: ≥ 15 of 20 testers complete ≥ 1 session and submit feedback
- G9-04: Week 1 crash rate: < 1 crash per 10 user-hours
- G9-05: Week 1 import success rate: ≥ 90% on beta musicians' personal styles
- G9-06: Week 1 chord recognition satisfaction: ≥ 4/5 (from "often correct" or "always correct")
- G9-07: Week 1 A/B test (3 musicians): average similarity score ≥ 3/5
- G9-08: Beta community channel active (≥ 10 discussions in first week)
- G9-09: All P0 bugs from week 1 fixed or planned with SLA (fix within 24h)
- G9-10: Bi-weekly check-in rhythm established (video call schedule for cohort)

**Deliverables:**
- G9-D1: TestFlight beta build (v1.0-beta-1)
- G9-D2: Beta onboarding package (welcome email, quick start, known issues, feedback form link)
- G9-D3: Beta community channel (Discord or Telegram)
- G9-D4: Week 1 beta report (usage stats, crash rate, issues found)
- G9-D5: Beta bug triage log (P0/P1/P2/P3 with SLA targets)
- G9-D6: Bi-weekly check-in schedule (next 6 weeks)

**Tests Required:**
- T-G9-01: Beta build: install via TestFlight on all reference devices
- T-G9-02: Crashlytics: verify crash report is received and visible
- T-G9-03: Feedback form: test submission + data export
- T-G9-04: Community channel: test invite link, verify notification delivery

**Evidence Required:**
- E-G9-01: TestFlight beta build ready and installed on test devices
- E-G9-02: Crashlytics dashboard: showing test crashes (verify pipeline)
- E-G9-03: Feedback form responses (≥ 15 in first week = green)
- E-G9-04: Beta community channel active

**RACI:**
- R: QA/Timing Engineer (beta operations)
- R: Domain Expert (beta cohort lead, musician liaison)
- A: Product Manager (beta go-ahead)
- C: All engineers (P0 bug emergency response)
- I: All team

**Risks:**
- R-BETA-01: Beta musicians not active → set minimum participation; replace inactive testers in week 2
- R-BETA-02: P0 bug discovered during musician's live show → hotfix within 24h + personal apology
- R-BETA-03: Negative word-of-mouth from crash-prone beta → emphasize "beta" in all communications

**Kill Criteria:**
- KC-G9-01: < 10 of 20 testers active after 2 weeks → beta participation critical mass not reached
- KC-G9-02: Crash rate > 1 per 5 user-hours → app not stable enough for external test

**Decision: [ ] Go / [ ] No-Go / [ ] Pivot**

---

## GATE 10 — Commercial Launch Readiness

**Objective:** App Store submission. All legal, marketing, and product readiness criteria met. The app is ready for public launch.

**Entry Criteria:**
- ✅ G9 beta complete — all exit criteria met
- ✅ Beta musician validation: ≥ 18/20 pass "2h show without crash"
- ✅ Beta musician validation: 0 section switch timing complaints in final 2 weeks
- ✅ App name trademark-clear (US + EU + JP)
- ✅ ToS + Privacy Policy finalized and published
- ✅ App Store metadata ready (name, subtitle, description, keywords, screenshots, preview video)
- ✅ All THIRD_PARTY_LICENSES.txt verified
- ✅ Marketing assets ready (demo video, tutorial videos, forum posts)

**Exit Criteria:**
- G10-01: App Store submission submitted (expect 1-2 weeks review)
- G10-02: App Store metadata complete and submitted (no placeholder text)
- G10-03: Screenshots: 5 portrait + 5 landscape, approved by designer
- G10-04: App Preview video: 30-second demo, approved by Product Manager
- G10-05: Privacy Policy URL: live, accessible, covers all data practices
- G10-06: Support email: active, auto-responder configured
- G10-07: FAQ page: 10 most common questions answered (English + Vietnamese)
- G10-08: Known limitations document: published on website (English + Vietnamese)
- G10-09: YouTube: 3 tutorial videos published (Import, Live Play, Setlist)
- G10-10: Forum campaign: posts on Yamaha community forum, Facebook groups (soft launch)
- G10-11: App Store reviewed: approved or reject reason addressed within 48h
- G10-12: Post-launch hotfix plan: 24h SLA for P0 bugs, next-day build if needed
- G10-13: Phase 2 planning: AUv3 hosting, SFF2 advanced, Korg import — timeline drafted

**Deliverables:**
- G10-D1: App Store submission package (binary + metadata + screenshots + preview)
- G10-D2: Published Privacy Policy URL
- G10-D3: Published FAQ page
- G10-D4: 3 YouTube tutorial videos
- G10-D5: Known limitations document (public)
- G10-D6: Launch-day checklist (all items verified green before pressing Submit)
- G10-D7: Phase 2 roadmap (document, with 6-month timeline)
- G10-D8: Post-launch support plan (bug response SLA, community engagement cadence)

**Tests Required:**
- T-G10-01: App Store review dry run: install app from TestFlight, full functionality check
- T-G10-02: Privacy Policy: verify all data practices match actual app behavior
- T-G10-03: Trademark: final sweep of all app copy, marketing copy, description
- T-G10-04: OSS license sweep: verify THIRD_PARTY_LICENSES.txt matches shipped code
- T-G10-05: Performance snapshot: final benchmark on reference hardware (store in repo)

**Evidence Required:**
- E-G10-01: App Store submission confirmation screenshot
- E-G10-02: Privacy Policy URL (live, accessible, correct)
- E-G10-03: YouTube videos published (viewable links)
- E-G10-04: Support email auto-responder test
- E-G10-05: Trademark search clearance certificates (if applicable)
- E-G10-06: Final beta exit report (18/20 musicians pass, 0 timing complaints)
- E-G10-07: Phase 2 roadmap document

**RACI:**
- R: Commercial/Product Owner (submission, marketing)
- A: Product Manager (launch decision)
- C: Legal/IP Counsel (trademark + ToS final review)
- C: All engineers (bug fixes, performance verification)
- I: All team

**Risks:**
- R-LAUNCH-01: App Store rejection → address issues, resubmit within 48h; buffer 2 weeks in launch timeline
- R-LAUNCH-02: Negative initial reviews → respond within 24h, fix P0 issues in 1-week hotfix
- R-LAUNCH-03: Server load from preview video → no server in MVP, all content is YouTube
- R-LAUNCH-04: Feature request overload post-launch → managed by disciplined backlog

**Kill Criteria:**
- KC-G10-01: App Store rejects for legal reason (trademark, copyright) → non-negotiable stop until resolved
- KC-G10-02: P0 bug found in final test that causes crash in 2h → must fix before submission

**Decision: [ ] Go / [ ] No-Go / [ ] Pivot**

---

# D. RACI MATRIX

## Roles Definition

| Role | Abbreviation | Description |
|---|---|---|
| **Product Manager** | PM | Product requirements, feature prioritization, beta management, market strategy |
| **Tech Lead** | TL | Architecture decisions, design reviews, code quality, technical risk management |
| **Audio/MIDI Engineer** | AE | C++ realtime audio engine, MIDI I/O, arranger logic, style parser — the "engine room" |
| **Swift/iOS Engineer** | IE | SwiftUI UI, Swift/C++ bridge, SQLite/GRDB library, setlist management |
| **C++/JUCE Engineer** | CE | (Same person as Audio/MIDI Engineer in MVP; listed separately for clarity) |
| **QA/Timing Engineer** | QA | Test automation, timing stress testing, stability/performance benchmarks, beta QA |
| **Legal/IP Counsel** | LC | Trademark search, ToS review, OSS compliance, IP risk assessment |
| **DevOps/Release** | DR | CI/CD pipeline, TestFlight distribution, App Store submission, build infrastructure |
| **Musician Tester** | MT | Domain expert: A/B musicality testing, beta cohort lead, acceptance criteria validation |
| **Commercial/Product Owner** | CO | Pricing, marketing, App Store metadata, revenue strategy, launch execution |

---

## Workstream RACI Matrix

| Activity | PM | TL | AE | IE | CE | QA | LC | DR | MT | CO |
|---|---|---|---|---|---|---|---|---|---|---|
| **WS-01: Product & Musician Workflow** | | | | | | | | | | |
| PRD maintenance & prioritization | **A** | C | C | C | C | C | - | - | C | C |
| Feature backlog management | **R** | C | C | C | C | C | - | - | C | C |
| Acceptance criteria definition | **R** | C | C | C | C | C | C | - | C | C |
| Beta feedback protocol | **R** | - | - | - | - | C | - | - | C | C |
| Competitive analysis | **R** | C | C | - | - | - | - | - | C | C |
| Pricing model | C | - | - | - | - | - | - | - | C | **R** |
| User persona validation | **R** | - | - | - | - | - | - | - | C | C |
| **WS-02: Realtime Audio/MIDI Engine** | | | | | | | | | | |
| JUCE framework integration | - | C | **R** | - | **R** | - | - | - | - | - |
| CoreAudio render callback | - | C | **R** | - | **R** | - | - | - | - | - |
| Lock-free queue implementation | - | C | **R** | - | **R** | - | - | - | - | - |
| MusicalClock | - | C | **R** | - | **R** | - | - | - | - | - |
| CoreMIDI device management | - | C | **R** | C | - | - | - | - | - | - |
| MIDI send thread + routing | - | C | **R** | - | - | - | - | - | - | - |
| MIDI initialization sequence | - | C | **R** | - | - | - | - | - | - | C |
| Latency benchmark | C | C | **R** | - | - | C | - | - | - | - |
| **WS-03: Arranger Runtime Logic** | | | | | | | | | | |
| Section state machine | - | C | **R** | - | **R** | - | - | - | C | - |
| NTR/NTT transform pipeline | - | C | **R** | - | **R** | - | - | - | C | - |
| Retrigger semantics | - | C | **R** | - | **R** | - | - | - | C | - |
| Render loop | - | C | **R** | - | **R** | - | - | - | - | - |
| Command queue processor | - | C | **R** | - | **R** | - | - | - | - | - |
| Style loader (atomic swap) | - | C | **R** | - | **R** | C | - | - | - | - |
| Unit test suite (transform, state machine) | - | C | **R** | - | **R** | C | - | - | C | - |
| A/B musicality test | C | C | **R** | - | - | C | - | - | **R** | - |
| **WS-04: Internal Style Format (UASF)** | | | | | | | | | | |
| UASF spec document | C | **R** | C | C | C | C | C | - | C | - |
| JSON schema file | - | **R** | C | C | C | C | - | - | - | - |
| UASF reader/writer library | - | C | **R** | - | **R** | - | - | - | - | - |
| ZIP container utility | - | C | **R** | C | **R** | - | - | - | - | - |
| Validation engine (V01-V07) | - | C | **R** | - | **R** | C | - | - | - | - |
| Migration framework | - | C | **R** | - | **R** | C | - | - | - | - |
| Chord type enum (shared) | - | **R** | C | C | C | - | - | - | - | - |
| **WS-05: Importer / Compatibility Layer** | | | | | | | | | | |
| SFF1 binary parser | - | C | **R** | - | **R** | C | - | - | C | - |
| SFF2 binary parser | - | C | **R** | - | **R** | C | - | - | - | - |
| UASF builder | - | C | **R** | - | **R** | C | - | - | - | - |
| Tick normalizer | - | C | **R** | - | **R** | C | - | - | - | - |
| ImportResult + limitation system | - | C | **R** | - | **R** | C | - | - | C | - |
| Yamaha→UASF mapping doc | - | C | **R** | - | **R** | - | C | - | C | - |
| Test corpus collection | C | - | C | - | - | **R** | - | - | **R** | - |
| Corpus test automation | - | - | C | - | - | **R** | - | C | - | - |
| A/B test harness | - | C | **R** | - | **R** | C | - | - | C | - |
| **WS-06: iPad Live UX** | | | | | | | | | | |
| Live Screen UI | C | - | - | **R** | - | C | - | - | C | C |
| Performance mode | C | - | - | **R** | - | C | - | - | C | - |
| Mixer panel | C | - | - | **R** | - | C | - | - | C | - |
| Style Browser | C | - | - | **R** | - | C | - | - | C | - |
| Setlist management | C | - | - | **R** | - | C | - | - | C | - |
| Import flow UI | C | - | - | **R** | - | C | - | - | C | - |
| On-screen chord pad | C | - | - | **R** | - | C | - | - | C | - |
| Visual design system | C | - | - | **R** | - | - | - | - | C | C |
| Haptic feedback | C | - | - | **R** | - | C | - | - | C | - |
| Error presentation system | C | - | - | **R** | - | C | - | - | C | - |
| **WS-07: MIDI Device Integration** | | | | | | | | | | |
| USB MIDI integration | - | - | **R** | C | - | C | - | - | C | - |
| BLE MIDI integration | - | - | **R** | C | - | C | - | - | C | - |
| Network MIDI integration | - | - | **R** | C | - | C | - | - | - | - |
| Virtual MIDI port | - | - | **R** | C | - | C | - | - | - | - |
| Device initialization sequence | - | - | **R** | C | - | C | - | - | C | - |
| Hot-plug notification + reconnect | - | - | **R** | C | - | C | - | - | C | - |
| Device routing configuration | C | C | **R** | C | - | C | - | - | C | - |
| Hardware compatibility test | - | - | **R** | - | - | C | - | - | C | - |
| **WS-08: Test Harness / Timing Lab** | | | | | | | | | | |
| CI pipeline configuration | - | C | - | - | - | C | - | **R** | - | - |
| GoogleTest + XCTest setup | - | C | C | C | C | **R** | - | C | - | - |
| Chord Engine test suite | - | C | **R** | - | - | C | - | - | - | - |
| NTR/NTT transform test suite | - | C | **R** | - | - | C | - | - | - | - |
| State machine test suite | - | C | **R** | - | - | C | - | - | - | - |
| Timing stress test harness | - | C | C | - | - | **R** | - | - | - | - |
| Memory stability test | - | - | C | C | - | **R** | - | - | - | - |
| Hardware latency benchmark | - | - | **R** | - | - | C | - | - | - | - |
| A/B musicality test protocol | C | C | C | - | - | **R** | - | - | C | - |
| CI nightly report | - | - | - | - | - | **R** | - | C | - | - |
| **WS-09: Legal / IP / Compliance** | | | | | | | | | | |
| IP Risk Assessment | C | C | C | C | C | - | **R** | - | - | C |
| Reverse-engineering legal memo | C | C | C | - | - | - | **R** | - | - | - |
| No DRM Circumvention Policy | **A** | C | C | C | C | - | **R** | - | - | C |
| Trademark search | C | - | - | - | - | - | **R** | - | - | **A** |
| App name approval | **A** | - | - | - | - | - | **R** | - | - | C |
| ToS + Privacy Policy | C | - | - | - | - | - | **R** | - | - | **A** |
| OSS license audit | - | C | C | C | C | C | **R** | - | - | - |
| GeneralUser GS license verification | - | - | C | C | C | - | **R** | - | - | - |
| Marketing copy review | C | - | - | - | - | - | C | - | - | **R** |
| **WS-10: DevOps / Build / CI / Release** | | | | | | | | | | |
| Xcode project + target configuration | - | C | C | C | C | - | - | **R** | - | - |
| CMake build system | - | C | C | - | C | - | - | **R** | - | - |
| CI pipeline (fast + nightly) | - | C | - | C | - | C | - | **R** | - | - |
| TestFlight distribution | - | - | - | C | - | C | - | **R** | - | - |
| Crashlytics integration | - | - | - | C | - | C | C | **R** | - | - |
| SwiftLint + clang-tidy | - | C | C | C | C | - | - | **R** | - | - |
| App Store Connect configuration | - | - | - | C | - | - | - | **R** | - | **A** |
| Build versioning + auto upload | - | - | - | - | - | - | - | **R** | - | - |
| Hotfix process document | C | C | C | C | C | C | - | **R** | - | **A** |
| **WS-11: QA / Musician Validation** | | | | | | | | | | |
| Pre-beta QA checklist | C | C | - | - | - | **R** | - | - | C | C |
| Beta program management | C | - | - | - | - | **R** | - | C | C | C |
| Beta feedback protocol execution | C | - | - | - | - | **R** | - | - | C | C |
| Bug triage (P0/P1/P2/P3) | **A** | C | C | C | C | **R** | - | - | C | C |
| A/B musicality test execution | - | - | C | - | - | C | - | - | **R** | - |
| Corpus expansion | - | - | - | - | - | **R** | C | - | C | - |
| 2h stability test execution | - | - | C | C | - | **R** | - | - | - | - |
| Known limitations document | C | C | C | C | C | **R** | C | - | C | C |
| Exit beta criteria checklist | C | C | C | C | C | **R** | - | - | C | **A** |
| **WS-12: Commercial Packaging** | | | | | | | | | | |
| Pricing model finalization | C | - | - | - | - | - | C | - | C | **R** |
| App Store metadata (description, keywords) | C | - | - | - | - | - | C | C | - | **R** |
| Screenshots + preview video | C | - | - | C | - | - | - | C | - | **R** |
| Privacy Policy URL | C | - | - | - | - | - | **R** | - | - | **A** |
| Support infrastructure | C | - | - | - | - | C | - | - | - | **R** |
| Demo video + YouTube tutorials | C | - | - | - | - | - | - | - | C | **R** |
| Marketing launch plan | C | - | - | - | - | - | - | - | - | **R** |
| Pre-launch checklist | **A** | C | C | C | C | C | C | C | C | **R** |
| Launch post-mortem | **A** | C | C | C | C | C | - | - | C | **R** |

---

## Gate RACI (Go/No-Go Decision)

| Gate | Responsible | Accountable | Consulted | Informed |
|---|---|---|---|---|
| **G0** — Project Sanity | Tech Lead | Product Manager | All engineers, Domain Expert | All team |
| **G1** — Timing Spike | Audio/MIDI Engineer | Tech Lead | QA, Domain Expert | PM, iOS Engineer |
| **G2** — Vertical Slice | Audio/MIDI Engineer | Tech Lead | Domain Expert | PM, iOS Engineer, QA |
| **G3** — UASF Format | Tech Lead | Tech Lead | All engineers | PM, QA |
| **G4** — SFF1 Importer | Audio/MIDI Engineer | Tech Lead | QA, Domain Expert | PM, iOS Engineer |
| **G5** — iPad UX | iOS Engineer | Product Manager | Designer, Domain Expert | QA, Tech Lead |
| **G6** — HW Compatibility | Audio/MIDI Engineer | QA/Timing Engineer | Domain Expert | PM, iOS Engineer |
| **G7** — Musician Alpha | QA/Timing Engineer | Product Manager | All engineers, Domain Expert | All team |
| **G8** — Stability Hardening | All engineers | Tech Lead | QA | PM |
| **G9** — Beta Package | QA/Timing Engineer | Product Manager | All engineers, Domain Expert | All team |
| **G10** — Commercial Launch | Commercial Owner | Product Manager | Legal, All engineers | All team |

---

## RACI Legend

| Letter | Meaning | Role in this activity |
|---|---|---|
| **R** = Responsible | Does the work | The person who executes the task |
| **A** = Accountable | Owns the outcome | The "buck stops here" person; must sign off |
| **C** = Consulted | Provides input | Consulted before decision/work is finalized |
| **I** = Informed | Needs to know | Told after decision/work is done |

---

# APPENDICES

## Appendix A: Risk Register Summary

| ID | Risk | Probability | Impact | Score | Gate | Owner |
|---|---|---|---|---|---|---|
| R01 | Cannot hire C++ audio engineer | High | Critical | 🔴 | G0 | Founder |
| R02 | SFF1 CASM bit-level wrong | Medium | High | 🟡 | G0 | AE |
| R03 | Timing jitter on iPad > expected | Medium | High | 🟡 | G1 | AE |
| R04 | sfizz insufficient for phase 2 | Low | Medium | 🟢 | G5 | AE |
| R05 | JUCE license change | Low | Medium | 🟢 | G0 | TL |
| R06 | Apple reject App Store | Low | High | 🟡 | G10 | LC |
| R07 | Yamaha C&D trademark | Low | High | 🟡 | G9 | LC |
| R08 | Beta crash in real show | Medium | Critical | 🔴 | G9 | QA |
| R09 | SFF2 import quality complaints | Medium | Medium | 🟡 | G4 | PM |
| R10 | vArranger iOS competitor | Low | High | 🟡 | G10 | CO |
| R11 | User WTP lower than expected | Medium | Medium | 🟡 | G9 | CO |
| R12 | iOS update breaking CoreMIDI | Low | High | 🟡 | G6 | IE |
| R13 | Domain expert lacks time | Medium | Medium | 🟡 | G0 | PM |
| R14 | Copyright issues in test corpus | Low | Low | 🟢 | G0 | QA |
| R15 | Memory leak in 2h session | Medium | Medium | 🟡 | G8 | AE |
| R16 | NTT music theory baseline wrong | Medium | High | 🟡 | G7 | AE |
| R17 | Beta musicians not active | Medium | Medium | 🟡 | G9 | QA |
| R18 | **Bundling proprietary Yamaha/Korg sample content (legal HIGH)** | Low | Critical | 🔴 | G3 | LC |
| R19 | **Marketing language implying official sound distribution (legal HIGH)** | Medium | Critical | 🔴 | G9 | CO |
| R20 | User-uploaded copyrighted samples in SF2/SFZ | Medium | Medium | 🟡 | G9 | LC |
| R21 | GM preview sound quality generates negative reviews | Medium | Medium | 🟡 | G10 | CO |
| R22 | MegaVoice detection false positives limit user playback | Medium | Medium | 🟡 | G4 | AE |

## Appendix B: Critical Path Timeline (Weeks 1-36)

```
Gate   Weeks   Focus Area                     Key Deliverable
─────────────────────────────────────────────────────────────────
G0     1-4    Foundation + De-risk            Spike M0 + audio stack
G1     5-6    Timing Validation               MusicalClock proven < 2 tick
G2     7-10   Vertical Slice                  Full chord→transform→MIDI out
G3     11-12  UASF Format Lock               UASF v1.0 spec + round-trip verified
G4     13-16  Import Pipeline                 SFF1 ≥ 95%, SFF2 ≥ 75%
G5     17-20  Live UX Prototype               Live Screen + Mixer + Bridge
G6     21-22  Hardware Lab                    10+ MIDI devices tested
G7     23-24  Musician Alpha                  3 × 2h session, 0 crash
G8     25-26  Stability Hardening             ASan/TSan clean, 0 crash 2h × 3
G9     27-32  Beta Program (6 weeks)          20 musicians, real shows
G10    33-36  Commercial Launch               App Store submission + marketing
```

## Appendix C: Architecture Decision Records (ADR) Index

| ID | Decision | Status |
|---|---|---|
| ADR-001 | iPad-first, not macOS-first | ✅ Closed (G0) |
| ADR-002 | JUCE for audio/MIDI backbone | ✅ Closed (G0) |
| ADR-003 | MIDI output before internal sound | ✅ Closed (G0) |
| ADR-004 | UASF = zip container (JSON + SMF hybrid) | ✅ Closed (G0) |
| ADR-005 | Audio clock = sole time source | ✅ Closed (G1) |
| ADR-006 | noteRanges array from v1.0 for SFF2 readiness | ✅ Closed (G0) |
| ADR-007 | sfizz over FluidSynth (iOS license) | ✅ Closed (G0) |
| ADR-008 | Swift/C++ interop over ObjC wrapper | ✅ Closed (G0) |
| ADR-009 | CoreMIDI direct (bypass JUCE MIDI wrapper) | ✅ Closed (G0) |
| ADR-010 | SQLite + GRDB for library metadata | ✅ Closed (G0) |
| ADR-011 | NTT table generation by chord theory, refined by A/B test | 🔄 Open (G2) |
| ADR-012 | Lock-free SPSC ring buffers for all cross-thread comms | ✅ Closed (G1) |
| ADR-013 | User-Loaded Sound Library & External Playback Policy | ✅ Approved (G0) |

---

## Appendix D: Key Metrics Dashboard

| Metric | Target | Measured At | Gate |
|---|---|---|---|
| Chord→MIDI latency P95 | ≤ 20ms | G1 | 🔴 Kill if > 50ms |
| Section switch timing MAX | ≤ 2 ticks | G2, G8 | 🔴 Kill if > 5 ticks |
| SFF1 import success | ≥ 95% | G4 | 🔴 Kill if < 80% |
| SFF2 import success | ≥ 75% | G4 | 🟡 Warn if < 60% |
| 2h stability crash count | 0 | G7, G8 | 🔴 Kill if any crash |
| Chord recognition satisfaction | ≥ 4/5 | G7 | 🟡 Warn if < 3/5 |
| A/B musicality score | ≥ 3.5/5 | G7 | 🟡 Warn if < 3/5 |
| Import throughput | < 2s/style | G4 | 🟡 Warn if > 5s |
| Audio callback CPU | < 40% | G8 | 🟡 Warn if > 60% |
| App memory (idle) | ≤ 80MB | G8 | 🟡 Warn if > 120MB |
| App memory (loaded) | ≤ 200MB | G8 | 🟡 Warn if > 300MB |
| UI tap response P95 | ≤ 100ms | G5 | 🟡 Warn if > 200ms |
| App Store review | Approved | G10 | 🔴 Kill if rejected (legal) |
| Beta exit: 2h crash-free | ≥ 18/20 | G9 | 🟡 Warn if < 15/20 |
| External MIDI device validation | ≥ 10 devices PnP | G6 | 🟡 Warn if < 8 devices |
| Yamaha external articulation pass-through | Correctly pass articulation data | G6 | 🟡 Warn if incomplete |
| MegaVoice detection accuracy | ≥ 90% on corpus | G4 | 🟡 Warn if < 75% |
| User consent dialog engagement | Shown on first import, never skipped | G9 | 🔴 Legal requirement |
| Bundled assets audit | Zero proprietary samples | G9 | 🔴 Legal requirement |
| Marketing terminology compliance | Zero violations | G10 | 🔴 Legal requirement |
| Degraded vs external playback gap | Documented for user clarity | G7 | 🟡 Warn if musically unacceptable |

---

*End of Master Delivery Plan — AI Arranger / Style Maker v1.0*

---

> **This document is the single source of truth for execution.**
> All workstream leads must read, understand, and follow this plan.
> Deviations from the gate structure require formal re-review and sign-off by the Product Manager.
>
> Last Updated: 2026-06-13  
> Next Review: At Gate 0 completion  
> **Revision Note:** v2.0 — Revised per ADR-013 (User-Loaded Sound Library & External Playback Policy). See ADR-013 for full policy details.
