# Musical Review Template

> **Required for:** All Gate 4+ PRs affecting arranger behavior, style import, chord processing, or articulation
> **Reviewer:** Musician / Domain Expert
> **Status:** PENDING / PASS / FAIL

---

## 1. Fill Transition Feel

| Section Transition | Smooth? | Cut? | Timing Correct? | Notes |
|-------------------|---------|------|-----------------|-------|
| Intro → Main | □ / □ | □ / □ | □ / □ | |
| Main → Fill | □ / □ | □ / □ | □ / □ | |
| Fill → Main | □ / □ | □ / □ | □ / □ | |
| Main → Ending | □ / □ | □ / □ | □ / □ | |

## 2. Chord Correctness

| Aspect | Result | Notes |
|--------|--------|-------|
| Root note matches input chord | □ Yes □ No | |
| Chord voicing (3rd, 5th, 7th) correct | □ Yes □ No | |
| Bass note follows root or inversion | □ Yes □ No | |
| Tension notes handled acceptably | □ Yes □ No | |
| No-cluster voicing violation | □ Yes □ No | |

## 3. Groove Preservation

| Element | Preserved? | Notes |
|---------|-----------|-------|
| Drum pattern feel | □ Yes □ No □ N/A | |
| Bass line groove | □ Yes □ No □ N/A | |
| Chord rhythm timing | □ Yes □ No □ N/A | |
| Tempo stability | □ Yes □ No | |
| Swing/shuffle if applicable | □ Yes □ No □ N/A | |

## 4. Articulation Fidelity

| Articulation Type | Preserved | Degraded | Lost | Notes |
|------------------|-----------|----------|------|-------|
| Staccato | □ | □ | □ | |
| Legato | □ | □ | □ | |
| Mute | □ | □ | □ | |
| Slap (bass) | □ | □ | □ | |
| MegaVoice | □ | □ | □ | |
| Keyswitch | □ | □ | □ | |
| Velocity-switch | □ | □ | □ | |

## 5. Arranger Behavior

| Behavior | Expected | Actual | Match? |
|----------|----------|--------|--------|
| Section switch on bar boundary | Bar X | Bar Y | □ Yes □ No |
| Fill length | 1 bar | N bars | □ Yes □ No |
| Ending behavior | Smooth stop | Cut/Decay | □ Yes □ No |
| Panic response | Instant | Delay ms | □ Yes □ No |

## 6. Overall Verdict

| Category | Score (1-5) |
|----------|-------------|
| Fill transition feel | /5 |
| Chord correctness | /5 |
| Groove preservation | /5 |
| Articulation fidelity | /5 |
| Arranger behavior | /5 |
| **TOTAL** | **/25** |

**PASS if:** TOTAL >= 15 AND no single category < 2

**Musical Review Verdict: □ PASS □ FAIL □ CONDITIONAL**

**Reviewer Notes:**
