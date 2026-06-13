# ADR-013: User-Loaded Sound Library & External Playback Policy

| Field | Value |
|---|---|
| **ID** | ADR-013 |
| **Status** | ✅ Approved |
| **Date** | 2026-06-13 |
| **Owner** | Tech Lead + Legal/IP Counsel |
| **Supersedes** | ADR-003 (partially — extends MIDI-first doctrine with sound source policy) |
| **Related ADRs** | ADR-003 (MIDI-Out First), ADR-007 (sfizz over FluidSynth), ADR-009 (CoreMIDI direct) |

---

## Context

The AI Arranger platform targets professional and semi-professional musicians who own existing MIDI hardware (keyboards, sound modules, tone generators) and want a software arranger that integrates with their existing setup. The original architecture (ADR-003) correctly prioritised MIDI output over internal sound, but did not fully address:

1. What sound sources the app should bundle vs. expect from users.
2. How user-loaded sample libraries (SF2, SFZ, AUv3) should be supported.
3. How MegaVoice- and articulation-heavy tracks (Yamaha-specific) should be handled after import.
4. The legal boundaries around bundled proprietary sample content.
5. The commercial implications of sound source claims in marketing.

This ADR formalises the policy for all sound-related features, bundles, import behaviour, and playback modes.

---

## Decision

### 1. Sound Source Strategy — "User-Loaded, Not App-Bundled"

The app **MUST NOT** bundle or imply distribution of proprietary samples from any hardware manufacturer (Yamaha, Korg, Roland, Ketron, etc.).

**Permitted bundled sound sources:**
- GM-compatible SoundFont bank (e.g. GeneralUser GS or Fluid R3 GM) for **degraded preview** only
- Default bank size limited to < 50MB, single-layer, no proprietary mapping

**Prohibited bundled sound sources:**
- Yamaha XG SoundFonts (proprietary)
- Genos/PSR/Tyros voice banks
- Any Korg/Roland/Ketron ROM samples
- Any expansion pack content (.ppi, .cpi)

**User-loaded sound sources (must support in Phase 1–2):**
- SF2 (SoundFont 2) files — user-imported
- SFZ files — user-imported
- AUv3 instruments — hosted via AUv3 plugin API
- External MIDI devices — USB/BLE/Network MIDI tone generators
- Virtual MIDI ports — inter-app routing (AUM, GarageBand, etc.)

### 2. User Consent & Licensing

**Every import of user sound files MUST have implicit or explicit consent:**
- ToS clause: "User warrants they own or have appropriate license for all imported sound files."
- Legal-banner on first import of SF2/SFZ: confirmation dialog stating licensing responsibility.
- Logging of consent acceptance (persistent, not per-session).

**Imported assets MUST be isolated:**
- User samples stored in app sandbox `Documents/UserSamples/` or `Application Support/UserSamples/`
- Never mixed with bundled app assets in the same directory
- App backup only backs up user samples with explicit user consent

### 3. Branding & Terminology

**The following terminology is CORRECT for all documentation:**
- "User-Loaded Sound Library Support"
- "External MIDI Device Playback"
- "AUv3 Instrument Hosting"
- "SF2/SFZ Import"
- "GM-Compatible Preview Bank"

**The following terminology is FORBIDDEN in commercial docs and marketing:**
- "Yamaha SoundFont Support" — ❌ replaced with "User-Loaded Sound Library Support"
- "Genos sounds included" — ❌ removed
- "PSR sound pack" — ❌ removed
- "Tyros voices" — ❌ removed
- Any phrasing implying bundled Yamaha/Korg/Roland sound content

**Documentation MUST use vendor-neutral language:**
- "Style Import" not "Yamaha Style Import" (but may reference Yamaha .sty as a supported format)
- "External Tone Generator" not "Yamaha Module" (but may list compatible devices)
- "Articulation Mapping" not "MegaVoice Emulation"

### 4. MegaVoice Handling Policy

MegaVoice tracks (Yamaha-specific articulations using velocity-switching across the MIDI key range) **MUST NOT be silently mapped to GM**.

**Required MegaVoice behaviours:**

| Aspect | Requirement |
|---|---|
| Detection | Importer MUST detect MegaVoice tracks (velocity range > 2 layers, note range spans > 40 keys, specific NTR patterns) |
| Preservation | All articulation metadata MUST be preserved in UASF (see ADR-004 extension below) |
| Visual indicator | UI MUST show "MegaVoice track detected — high-fidelity playback requires external Yamaha device or premium library" |

**Playback modes (strictly ordered by priority):**

| Mode | Description | Fidelity | Display |
|---|---|---|---|
| MODE 1 | External Yamaha Playback — route MIDI to connected Yamaha device | Highest | "External Device Playback" |
| MODE 2 | User-Loaded Premium Library — user provides custom SF2/SFZ/AUv3 with articulation mapping | High | "User Sound Library" |
| MODE 3 | Degraded Preview — GM fallback with note mapping | Low | "Low-fidelity fallback mode — NOT suitable for performance" |

**Critical rule:** Mode 3 (GM fallback) **NEVER counts as a success metric**, must display the warning text, and must be clearly distinguishable from high-fidelity modes in the UI.

### 5. UASF Schema Extension — Articulation Metadata

The UASF manifest schema is extended with the following optional fields (ADR-004 is amended accordingly):

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
  "external_device_map": {             // optional mapping data for external devices
    "bank_msb": 0,
    "bank_lsb": 0,
    "program": 0,
    "device_hint": "yamaha_genos"
  },
  "articulation_hints": {              // for vendor-neutral expressive intent
    "velocity_layers": 3,
    "note_thresholds": [60, 80, 100],
    "key_switch_range": {"low": 0, "high": 0}
  }
}
```

**Runtime engine rule:** The Arranger Engine and Audio Engine know only:
1. `articulation_profile` — how to schedule notes (may affect velocity mapping)
2. `fidelity_requirement` — whether GM fallback is acceptable
3. `recommended_playback` — ordered list of playback capability IDs
4. `rendering_mode` — determined at playback start by matching capabilities

**Critical rule:** The runtime engine MUST NOT contain any Yamaha-specific parsing or mapping. All MegaVoice/Yamaha-specific logic lives in the Import Adapter (offline). The runtime works with vendor-neutral articulation profiles.

### 6. Internal Sound Engine Scope Limitation

The app's internal sound engine (sfizz + GM bank) is explicitly scoped as:
- **Phase 1:** Degraded preview only — single layer, no velocity switching
- **Phase 2:** User-loaded SF2/SFZ — full layering, velocity mapping from user banks
- **Phase 3:** User-loaded AUv3 hosting — premium instrument integration
- **Phase 3+:** Premium (partner-created) sound libraries — marketplace model, never bundled in app binary

The app **MUST NOT attempt** to emulate or replicate Yamaha/Korg/Roland-specific sound characteristics. The sound engine is a conduit for user-provided content, not a synthesis of hardware ROM.

### 7. Commercial Moat Definition

The product's commercial moat is explicitly defined as:
- ✅ Arranger workflow quality (timing, NTT/NTR, section switching)
- ✅ Live performance UX (low-light, one-handed, glanceable)
- ✅ Import compatibility (SFF1/SFF2 → UASF pipeline)
- ✅ Timing quality (sub-tick accuracy)
- ✅ Reliability (zero crash, stable MIDI)
- ✅ Cloud ecosystem for user setlists and style library
- ✅ AI-assisted workflow (Phase 2+)
- ✅ Musician productivity (faster setup, better live experience)

**The commercial moat is NOT:**
- ❌ Bundled proprietary sounds
- ❌ Sound realism
- ❌ Manufacturer ROM content
- ❌ Claimed "sounds like Yamaha Genos"

### 8. Implementation Priority Order

The following priority order governs all sound-related implementation decisions:

| Priority | Area | Rationale |
|---|---|---|
| 1 | Timing accuracy | The one non-negotiable product value |
| 2 | MIDI stability | Reliability over features |
| 3 | Arranger correctness | NTT/NTR/section switch accuracy |
| 4 | Live UX | Stage-ready interface |
| 5 | Import compatibility | SFF1/SFF2 conversion reliability |
| 6 | External playback ecosystem | USB/BLE/Network MIDI device support |
| 7 | User-loaded sound libraries | SF2/SFZ/AUv3 import and playback |
| 8 | Internal articulation engine | GM-fallback quality improvements |
| 9 | Proprietary premium sound ecosystem | Phase 3 marketplace |

---

## Consequences

### Positive
- ✅ Legal risk severely reduced — no proprietary sample bundling
- ✅ Clear user expectations — musicians know they need external sound sources for high fidelity
- ✅ Marketing copy is defensible — no misleading claims about bundled sounds
- ✅ Future-proofed — AUv3 support and premium library marketplace are Phase 2/3 ready
- ✅ Musicians' existing hardware investment is respected — no forced upgrade path
- ✅ Focused engineering effort — no time wasted on Yamaha sound emulation

### Negative
- ❌ GM preview sound quality will disappoint some users in Phase 1
- ❌ MegaVoice tracks require external Yamaha device for best results — limitations must be clearly communicated
- ❌ User onboarding for sound setup is more complex (no "just works" sound)
- ❌ Marketing message is harder to simplify — "best with your existing keyboard" vs. "all-in-one"

### Risks
| Risk | Mitigation |
|---|---|
| User confusion about required sound setup | Clear onboarding wizard, setup guide, video tutorials |
| Negative reviews about sound quality in Phase 1 | Explicit "GM preview — for best sound connect your keyboard" in description |
| Legal grey area around user-uploaded SF2 samples | ToS + consent + DMCA takedown process |
| Performance hits from AUv3 hosting (Phase 3) | Dedicated AUv3 host process, audio-thread isolation |

---

## Compliance Checklist

All documentation, marketing material, and code must be audited against:

- [ ] No bundled proprietary samples from any manufacturer
- [ ] All "Yamaha SoundFont Support" → "User-Loaded Sound Library Support"
- [ ] No "Genos sounds included" / "PSR sound pack" / "Tyros voices" in any commercial doc
- [ ] MegaVoice detected tracks show correct UI indicators
- [ ] GM fallback displays "Low-fidelity fallback mode" — never counts as success metric
- [ ] UASF schema includes articulation metadata fields
- [ ] Runtime engine uses vendor-neutral articulation profiles only
- [ ] User consent dialog on first sound file import
- [ ] Imported sample assets isolated from bundled app assets
- [ ] Commercial moat documentation updated per this ADR

---

## References

- ADR-003: MIDI-Out First, Internal Sound Second
- ADR-004: UASF = ZIP Container (JSON + SMF Hybrid)
- ADR-007: sfizz Over FluidSynth (iOS License)
- AI_ARRANGER_MASTER_DELIVERY_PLAN.md — Sound Strategy Doctrine
- file-import-strategy.md — MegaVoice handling section
- live-arranger-strategy.md — Commercial moat definition
