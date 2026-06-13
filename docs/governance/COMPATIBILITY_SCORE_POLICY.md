# Compatibility Score Policy

> **Status:** ACTIVE  
> **Effective from:** Gate 4 (2026-06-13)  
> **Applies to:** All importer modules

---

## Definition

| Metric | Meaning | Target (SFF1) |
|--------|---------|----------------|
| **Parse success** | File reads without crash/error | >= 90% |
| **Structural fidelity** | Sections, tracks, bars, events match original | >= 85% |
| **Harmonic fidelity** | Chord behavior (NTT/NTR) matches expected | >= 70% |
| **Groove fidelity** | Timing, feel within 5% of reference | >= 75% |
| **Articulation fidelity** | MegaVoice, keyswitch, velocity mapping | >= 50% |
| **Playback usability** | Usable live without audible issues | Yes/No |

## Scoring Method

```
Parse success     = (files_parsed / total_files) * 100
Structural         = (matched_fields / total_fields) * 100
Harmonic           = (correct_chord_behaviors / total_tested) * 100
Groove             = (events_within_jitter_threshold / total_events) * 100
Articulation       = (preserved_articulations / total_articulations) * 100
Playback usability = subjective musician rating (yes/no)
```

## Merge Gates

| Score Range | Merge Status |
|-------------|-------------|
| All >= 80% | ✅ Can merge |
| Any < 80% >= 60% | ⚠️ Document limitations, merge with explicit PTH approval |
| Any < 60% | ❌ Cannot merge — experimental branch only |

## Reporting

Every importer PR must include a compatibility score report:

```markdown
## Compatibility Score Report

| Metric | Score | Target | Status |
|--------|-------|--------|--------|
| Parse success | 95% | >= 90% | ✅ |
| Structural | 88% | >= 85% | ✅ |
| Harmonic | 72% | >= 70% | ✅ |
| Groove | 80% | >= 75% | ✅ |
| Articulation | 55% | >= 50% | ✅ (degraded) |
| Playback usable | Yes | Yes | ✅ |

Known limitations:
- MegaVoice → GM fallback (documented, warning displayed)
```
