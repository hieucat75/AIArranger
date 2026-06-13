# GATE 4 — Yamaha SFF1 Importer Research Plan

> **Status:** RESEARCH — not coding yet  
> **Branch:** gate-4-yamaha-sff1-research  
> **Prerequisite:** Gate 3 ✅ (UASF v1 frozen)

---

## Objective

Research Yamaha SFF1 format structure and define importer boundary.
No full importer implementation in this gate.
No Yamaha/Korg logic in runtime.

---

## Non-Goals

- Full SFF1 parser (Gate 5+)
- SFF2 support
- Commercial compatibility claim
- Any vendor logic in runtime
- Copyrighted sample bundling
- DRM bypass
- Silent unsupported conversion

---

## Research Outputs

| # | Output | Deliverable |
|---|--------|-------------|
| R1 | SFF1 structure notes | `docs/importers/sff1-structure-notes.md` |
| R2 | Importer boundary design | `docs/importers/sff1-importer-architecture.md` |
| R3 | Corpus folder + sample metadata | `corpus/yamaha/` + `.meta.json` per file |
| R4 | Unsupported feature taxonomy | `docs/importers/sff1-unsupported-features.md` |
| R5 | MegaVoice detection strategy | `docs/importers/sff1-megavoice-strategy.md` |
| R6 | Legal/interoperability note | `docs/importers/sff1-legal-interop-note.md` |
| R7 | Preliminary parser spike plan | `docs/gate-plans/GATE_5_SFF1_PARSER_SPIKE.md` (draft) |

---

## Research Method

### 1. Binary inspection (offline)
- Use hexdump tools to analyze SFF1 header structure
- CASM section layout
- OTS (One Touch Setting) blocks
- Style section encoding (Intro/Main/Fill/Ending)

### 2. Differential analysis
- Compare multiple SFF1 files
- Identify fixed vs variable fields
- Document undocumented bytes

### 3. Interoperability boundary
- What CAN be imported safely (note data, structure)
- What CANNOT be imported (DRM, ROM, proprietary encoding)
- What requires user-owned file verification

### 4. Mapping to UASF
- Existing UASF fields: section, track, event, articulation
- Gaps: SFF1-specific features not in UASF
- Proposal: new optional UASF fields (if needed)

---

## Importer Architecture Design

```
SFF1 file → Binary reader → SFF1 Intermediate → Mapper → UASF StyleDefinition
                                  ↓
                          Unsupported features
                          (logged, rejected safely)
```

Key principle:
- **Parser produces intermediate format** (SFF1-specific, not UASF)
- **Mapper converts intermediate → UASF** (this is where vendor logic lives)
- **Runtime never sees the intermediate format**

---

## Timeline

| Phase | Duration | Output |
|-------|----------|--------|
| Research | 1 session | Structure notes, corpus setup |
| Boundary design | 1 session | Architecture doc, unsupported taxonomy |
| Legal review | async | Interop note |
| Spike plan | 1 session | Gate 5 plan draft |

---

## Exit Criteria

- [ ] All 7 research outputs created
- [ ] Corpus folder with at least 2 sample styles
- [ ] Importer boundary clearly defined
- [ ] Unsupported features documented
- [ ] MegaVoice detection strategy documented
- [ ] Legal note drafted
- [ ] No importer code in runtime
- [ ] No commercial compatibility claim
