# Clock Jitter Comparison (Gate 15)

> deterministic: true · hardware_validated: false (illustrative model; real numbers measured locally)

| clock | mean (samp) | max abs dev | stddev |
|---|--:|--:|--:|
| prototype 1ms timer | 52.250 | 17.750 | 10.841 |
| CoreAudio host-time | 48.000 | 1.000 | 0.707 |

Host-time reads true elapsed each poll -> no fixed-step drift -> much lower jitter.
