# KORG Validation Report — synthetic_two_bar (engine vs +5tick reference)

> **hardware_validated: false** (synthetic fixture; no real-device parity claimed) · harness v1

## Timing

| matched | orphan(eng) | orphan(ref) | within tol | mean ms | max|abs| ms | stddev ms |
|--:|--:|--:|--:|--:|--:|--:|
| 6 | 0 | 0 | 5 | 0.868 | 5.208 | 1.941 |

## Transitions

| label | expected | actual | delta | grid off | on boundary | on time |
|---|--:|--:|--:|--:|:--:|:--:|
| Main A (start) | 0 | 0 | 0 | 0 | yes | yes |
| Main A->B | 1920 | 1920 | 0 | 0 | yes | yes |

## Chord latency

| events | missing | mean ms | max ms |
|--:|--:|--:|--:|
| 2 | 0 | 0.000 | 0.000 |

## Stuck-note

| balanced | hanging | double-on | orphan-off |
|:--:|--:|--:|--:|
| yes | 0 | 0 | 0 |

## Jitter

| events | mean abs ms | p95 ms | max ms |
|--:|--:|--:|--:|
| 3 | 1.736 | 5.208 | 5.208 |
