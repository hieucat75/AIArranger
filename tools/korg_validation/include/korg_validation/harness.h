#ifndef KORG_VALIDATION_HARNESS_H
#define KORG_VALIDATION_HARNESS_H

// ── KORG Validation Harness (Gate 11) ────────────────────────────────
//
// Pre-hardware, CI-safe measurement tooling for the arranger engine. The
// harness is a vendor-agnostic CONSUMER of the engine + UASF + MIDI captures;
// it never modifies the engine and performs no realtime work.
//
// Hardware status: hardware validation is DEFERRED_BY_PTH. The harness runs on
// SYNTHETIC fixtures only — a green run proves the tooling + engine against a
// reference, NOT parity with any real device. No KORG/Yamaha compatibility is
// claimed anywhere. Every emitted report carries `hardware_validated: false`.
// See docs/strategy/KORG_FIRST_VALIDATION_STRATEGY.md and
// docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md.

namespace ai_arranger::korg_validation {

// Single source of truth for the claims policy. NEVER set true without committed
// real-device capture evidence (and a policy amendment via PR).
inline constexpr bool kHardwareValidated = false;

// Harness format version (bumped when the report schema changes).
inline constexpr int kHarnessVersion = 1;

} // namespace ai_arranger::korg_validation

#endif // KORG_VALIDATION_HARNESS_H
