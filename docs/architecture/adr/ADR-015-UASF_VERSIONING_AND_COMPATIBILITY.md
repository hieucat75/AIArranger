# ADR-015: UASF Versioning and Compatibility

**Status:** ACCEPTED  
**Date:** 2026-06-13  
**Context:** Gate 3 established UASF as canonical internal format. Future gates will add importers, tooling, and style processing. Need explicit versioning and compatibility rules.

## Decision

1. UASF uses semver `MAJOR.MINOR.PATCH` (see `UASF_VERSIONING_POLICY.md`)
2. Golden file tests are stored in `tests/golden/uasf/`
3. All optional fields must use default-presence semantics
4. Unknown fields in MINOR versions must be preserved through roundtrip
5. No silent format evolution: every structural change requires version bump

## Consequences

- **Positive:** Long-term compatibility, safe importer evolution
- **Positive:** Golden files protect against regression
- **Negative:** Version check adds ~10 bytes to every UASF file
- **Negative:** Migration tooling needed for major changes

## Implementation

- Reader: check header.version, reject unknown MAJOR, warn unknown MINOR
- Writer: always writes current MAJOR.MINOR
- Golden tests: CI must pass all existing golden files on every PR
