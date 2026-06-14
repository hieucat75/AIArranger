# Korg Capture Fixtures

> **Empty until real hardware is available.** These directories will hold actual
> capture files recorded from a Korg PA700 / PA1000 when hardware validation is
> reactivated. The harness is **synthetic-only** today; nothing here implies a
> real device has been tested. See
> `docs/strategy/KORG_FIRST_VALIDATION_STRATEGY.md` and
> `docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md`.

## Layout

```
fixtures/korg/
├── PA700/    # real PA700 captures (empty placeholder)
└── PA1000/   # real PA1000 captures (empty placeholder)
```

## Capture format

Two interchangeable formats normalize to the same in-memory stream:

- **SMF (`.mid`)** — preferred for device captures (record the device's MIDI OUT).
- **CSV** — `tick,status,data1,data2` per line; `#` comments; an optional
  `ppqn=<n>` directive sets resolution; status/data may be decimal or `0xNN`.

## Naming convention

```
fixtures/korg/<MODEL>/<style>__<section>__<chord-seq>.mid
# example:
fixtures/korg/PA700/pop_acoustic__mainA__C-F-G-Am.mid
```

## Reactivation (when hardware is available)

1. Record the device MIDI OUT for the fixture progression into this directory.
2. Run `korg-validate --engine <engine.csv|mid> --reference <device.mid> \
   --out-json report.json --out-md report.md`.
3. Commit the captures + report.
4. Only then may compatibility claims be amended via PR (per the Claims Policy).

Synthetic, hand-authored fixtures used by the CI tests live separately under
`tools/korg_validation/tests/fixtures/synthetic/`.
