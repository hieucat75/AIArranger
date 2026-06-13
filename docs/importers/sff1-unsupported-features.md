# SFF1 Unsupported Features Taxonomy

> **Status:** DRAFT  
> **Branch:** gate-4-yamaha-sff1-research

---

## Classification

| Level | Behavior | Example |
|-------|----------|---------|
| **IMPORTABLE** | Full conversion to UASF without loss | Note events, section structure |
| **MAPPABLE** | Converted with documented limitations | MegaVoice → articulation profile |
| **DEGRADED** | Can be imported but fidelity reduced | GM drum mapping, velocity remap |
| **REJECTED** | Cannot be imported; error/warning | DRM, DSP parameters, encrypted sections |
| **IGNORED** | Silently skipped (no musical impact) | Padding bytes, reserved fields |

## Unsupported Features List

| Feature | Classification | Reason |
|---------|---------------|--------|
| DSP effect parameters | REJECTED | Proprietary Yamaha DSP |
| OTS voice selection | IGNORED | Only relevant for Yamaha hardware |
| Reverb/chorus settings | IGNORED | MIDI CC 91/93 can approximate |
| SFF2 compressed sections | REJECTED | Requires SFF2 decoder |
| Custom NTT tables | DEGRADED | Will use default table; fidelity warning |
| Encrypted styles | REJECTED | DRM; cannot process |
| MultiPad definitions | DEGRADED | Mapped to Phrase tracks |
| Hardware-specific SysEx | REJECTED | Yamaha-specific |
| MegaVoice mapping | DEGRADED | Articulation profile preserved; external device recommended |
| CASM velocity tables | DEGRADED | Mapped to articulation metadata |
| Section name encoding | IGNORED | Shift-JIS not in UASF scope |
| Keyboard range (HiKey/LoKey) | MAPPABLE | Note range filter in UASF (future field) |

## Behavior on Unsupported

Each rejection produces:
1. Structured log entry: `[SFF1-IMPORT] REJECTED: DSP parameters in section Main A`
2. Warning in compatibility report
3. No crash, no silent data loss

## MITIGATION

For DEGRADED features, the mapper:
- Preserves as much metadata as possible in UASF articulation fields
- Sets `fidelity = Low` in the UASF track
- Populates `playback_recommendations` to suggest external device
