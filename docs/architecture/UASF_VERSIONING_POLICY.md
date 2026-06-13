# UASF Versioning Policy

> **Status:** ACTIVE  
> **Effective from:** Gate 3 (2026-06-13)  
> **Applies to:** All `.uasf` binary files + in-memory StyleDefinition

## Semantic Versioning

UASF uses semantic versioning: `MAJOR.MINOR.PATCH`

| Level | Change | Backward Compat | Reader Behavior |
|-------|--------|----------------|-----------------|
| **MAJOR** | Breaking: field removed, type changed, semantics changed | ❌ No | Must refuse to load old format |
| **MINOR** | Extension: new field/type/section added (optional) | ✅ Yes | Must read old files; must ignore unknown fields |
| **PATCH** | Fix: metadata, documentation, tooling only | ✅ Yes | No structural change |

## Rules

1. **MAJOR increment** requires:
   - Migration guide
   - Conversion tool (old → new)
   - Minimum 1 release cycle deprecation notice

2. **MINOR increment** requires:
   - New fields MUST be optional (reader must handle absence)
   - unknown fields MUST be preserved (writer must roundtrip)
   - Default values for new fields MUST match old behavior

3. **PATCH increment** requires:
   - No structural field changes
   - No semantic changes to existing fields

## Current Version

- **v0.1 (unofficial):** Gate 2 internal demo (no file format)
- **v1.0:** Gate 3 binary format (first official)

## Backward Compatibility Commitment

From v1.0 onward:
- All UASF files written by any future version must be readable by v1.0 reader
- New optional fields are allowed; existing fields frozen
- No reordering of sections/tracks/events without version bump
