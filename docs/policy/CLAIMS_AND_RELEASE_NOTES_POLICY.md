# Claims & Release Notes Policy

Enforces honest scoping for releases.

## Hardware Validation

Until real-device validation evidence exists in the repo:
- Release notes / tag annotations MUST include "Hardware validation pending".
- No documentation, README, marketing material, demo claim, or PR description may
  state "compatible with Yamaha/Korg" or equivalent.
- Allowed phrasing examples:
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
