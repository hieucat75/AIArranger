# Claims & Release Notes Policy

Enforces honest scoping for releases.

## Hardware Validation

Applies to **all** hardware targets — Korg (the KORG-first validation target:
PA700/PA1000) and Yamaha alike. See
`docs/strategy/KORG_FIRST_VALIDATION_STRATEGY.md`.

Until real-device validation evidence exists in the repo:
- Release notes / tag annotations MUST include "Hardware validation pending".
- No documentation, README, marketing material, demo claim, or PR description may
  state "compatible with Korg/Yamaha" (or any specific device) or equivalent.
- Allowed phrasing examples:
  - "Engine-side arranger platform (KORG-first validation target, hardware parity
    unverified)"
  - "Engine-side Yamaha SFF/CASM importer (hardware parity unverified)"
  - "Parses Yamaha SFF1 style files (compatibility claim deferred)"
  - "Korg MIDI output supported (musical parity validation pending)"

## Future Milestone

When hardware review is reactivated:
- PTH or future maintainer runs `docs/gate-plans/GATE_10B_HARDWARE_CHECKLIST.md` +
  `docs/gate-plans/GATE_10B_NTT_CALIBRATION.md` on a real device.
- Evidence committed to repo (scorecards filled, NTT delta tables).
- Then claims policy may be amended via PR.

> Current status: **HARDWARE_REVIEW_STATUS: DEFERRED_BY_PTH (2026-06-14)** — see
> `.openclaw/handoff/HANDOFF_MASTER.md`.

## Enforcement

- PR reviewers must check this policy before merging anything that references
  Yamaha/Korg.
- README/docs/release-notes drift caught in audit (manual or future linter).
