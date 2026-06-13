# SFF1 MegaVoice Detection Strategy

> **Status:** DRAFT  
> **Branch:** gate-4-yamaha-sff1-research  
> **Alignment:** ADR-013 (User-Loaded Sound Policy)

---

## What Is MegaVoice?

Yamaha MegaVoice articulations use velocity ranges to trigger different playing styles on a single MIDI note:
- `v001-064` = normal pick
- `v065-090` = muted
- `v091-100` = harmonics
- `v101-127` = slide/noise

## Detection Strategy

### Phase 1 — Heuristic Detection (importer-time)

```
For each track in SFF1:
  1. Collect all NoteOn velocities
  2. If velocities span >= 3 distinct ranges (e.g., 0-60, 60-90, 90-127):
     → Mark as MegaVoice candidate
  3. If track name contains "MEGA" or "Mega":
     → Mark as MegaVoice candidate
  4. If CASM section indicates MegaVoice:
     → Mark as confirmed MegaVoice
```

### Phase 2 — Classification

| Classification | Action |
|---------------|--------|
| Confirmed MegaVoice | ArticulationProfile = MegaLike, fidelity = Medium |
| Suspected MegaVoice | ArticulationProfile = MegaLike, fidelity = Low + warning |
| Not MegaVoice | Normal articulation detection |

### Phase 3 — Playback Mode Routing

| Mode | Description | UASF Flag |
|------|-------------|-----------|
| External Yamaha | Route to Yamaha device | `recommended_playback.external_yamaha = true` |
| Premium Library | User-loaded SF2/AUv3 | `recommended_playback.premium_library = true` |
| Degraded GM | Fallback (with warning) | `degradation_allowed = true` + fidelity = Low |

## Key Principle

The runtime does NOT know what MegaVoice is.
It only sees:
- `articulation_profile = MegaLike`
- `fidelity = Medium/Low`
- `recommended_playback = [...]`

The heuristic detection and routing happen entirely in the importer layer.
