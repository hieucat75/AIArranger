# UASF Migration Policy

> **Status:** ACTIVE  
> **Effective from:** Gate 3 (2026-06-13)

## When Migration Is Required

Migration from one UASF version to another is required when:
- MAJOR version increment (breaking structural change)
- Field semantics change (even if layout unchanged)
- Compression or encryption introduced

## Migration Process

1. **Detect** — Reader checks `version` field in FileHeader
2. **Map** — Create conversion function: `old format → new format`
3. **Verify** — Roundtrip test with golden files
4. **Deprecate** — Old format still readable for N releases (N >= 1)
5. **Remove** — After deprecation window, drop old reader

## Golden File Tests

Golden files are stored in `tests/golden/uasf/`:
- One `.uasf` file per UASF format version
- Reader must successfully load and validate all golden files
- Any PR that changes parsing MUST update golden tests

## Migration Matrices

### v1.0 → v2.0 (hypothetical)

| Field | v1.0 | v2.0 | Migration |
|-------|------|------|-----------|
| tempo_bpm | uint32 in Style | uint32 in Section | Move + default |
| articulation | TrackHeader | new block | Auto-convert |

## No-Auto-Migration Zones

Some format changes MUST NOT auto-migrate:
- DRM/encryption flags
- Changes to NoteOn/Off semantics
- Tempo map changes affecting playback feel

These require explicit user action or full re-export.
