# KORG-First Validation Strategy

> Status: **ADOPTED 2026-06-14.** Supersedes the implicit "Yamaha-compatible
> arranger" framing. Hardware validation remains **DEFERRED_BY_PTH** — this
> document sets the *target and method* for when it reactivates, not a claim of
> current compatibility (see `docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md`).

---

## Context

The engine began as a Yamaha SFF1/SFF2 + CASM importer, which led to an implicit
"Yamaha-compatible arranger" framing. In practice the runtime is **vendor-
agnostic**: it speaks UASF (its own canonical format) and standard MIDI, and the
Yamaha specifics live only in the importer layer. The product is therefore not a
Yamaha clone — it is a **modern realtime arranger platform** that happens to
*import* Yamaha styles.

Hardware validation has been deferred. Before resuming it, we are choosing a
concrete, attainable first hardware target rather than the flagship devices the
old framing implied.

## Core Direction

**Modern realtime arranger platform with a KORG-first validation strategy.**

- The platform is software-defined and vendor-agnostic at runtime.
- The **first** real-hardware validation target is **Korg PA700 / PA1000**
  (mid-tier, widely available, MIDI-friendly).
- Yamaha (Genos/Tyros), Korg PA5X, and other flagship-tier devices are **not**
  initial targets.
- No compatibility with any device is claimed until recorded evidence exists.

## Rationale

### 1. Attainable, representative hardware
The Korg PA700/PA1000 are mid-tier arrangers that are affordable, easy to source,
and expose clean USB-MIDI. They exercise the same realtime concerns (section
switching, fills, chord-tracking latency, stuck-note safety) as flagship units
without flagship cost or scarcity — so validation can actually happen.

### 2. Vendor-agnostic engine, vendor-specific importers
The runtime already only understands UASF + MIDI; Yamaha/Korg knowledge is
isolated in the importer layer. Validating MIDI behaviour against a Korg proves
the *platform* without coupling the engine to any one vendor. Yamaha import
remains supported as a feature, not as the product's identity.

### 3. Infrastructure-first de-risking
The biggest validation risk is *measurement*, not the device. By building the
capture/measurement harness first (timing differential, jitter, transition
correctness) we can validate deterministically and repeatably the moment a device
is available — and the same harness later covers Yamaha or any other target.

## Hardware Policy

- **Primary validation target:** Korg PA700, Korg PA1000.
- **Not recommended initially:** Yamaha Genos, Yamaha Tyros, Korg PA5X, and other
  flagship-tier devices (cost, scarcity, over-spec for first validation).
- **Claims:** no "compatible with Korg/Yamaha" claim until recorded scorecard +
  timing/NTT evidence is committed (per the Claims Policy).
- **Status:** HARDWARE_REVIEW_STATUS = DEFERRED_BY_PTH (2026-06-14).

## Next Hardware Milestone

**Milestone — KORG Parity Validation (Korg PA700/PA1000).** When PTH signals
readiness:
1. Acquire/borrow a Korg PA700 or PA1000.
2. Run the validation harness (below) + the manual checklists
   (`GATE_10B_HARDWARE_CHECKLIST.md`, `GATE_10B_NTT_CALIBRATION.md`).
3. Commit recorded evidence (captures, timing tables, scorecards).
4. Only then may compatibility claims be amended via PR.

## Required Infrastructure Before Hardware Acquisition

Build these under `tools/korg_validation/` **before** buying hardware, so the day
a device arrives is measurement, not tooling:

- **MIDI capture** — record the device's MIDI OUT and our engine's output to a
  comparable, timestamped log format.
- **Timing differential** — align our dispatched events vs the device's events
  and report per-event delta (ms / ticks).
- **Section transitions** — verify section switches land on the expected bar
  boundary; flag early/late transitions.
- **Fill timing** — measure fill-in entry/exit alignment.
- **Chord latency** — time from chord input to first re-voiced note.
- **Panic / stuck-note** — assert all-notes-off leaves zero hanging notes.
- **Jitter** — distribution of inter-event timing error over a sustained run.
- **Parity harness** — orchestrates the above into a single repeatable
  pass/fail + report, reusable for any future hardware target.

> This infrastructure is a **future phase** — not implemented yet. It is the gate
> that precedes hardware acquisition.

## Long-Term Positioning

A **software-defined arranger instrument platform**: vendor-agnostic realtime
engine, importers for multiple vendor style formats, validated against real
hardware starting with Korg and expanding outward. The product's identity is the
platform and its musical quality — not emulation of any single manufacturer.
