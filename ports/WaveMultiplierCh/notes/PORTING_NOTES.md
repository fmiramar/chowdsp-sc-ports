# WaveMultiplierCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/WaveMultiplierCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
Port target: the chowdsp_utils `WaveMultiplier<float, 6>`, exposed as:

```supercollider
WaveMultiplierCh.ar(in, mul = 1, add = 0)
```

This scope includes:

1. the original six cascaded folder cells
2. the original second-order ADAA state update per stage
3. the resulting six-sample total latency

This scope does not yet include:

1. numeric comparison against chowdsp_utils
2. user-facing drive or output parameters beyond SC arithmetic
3. alternate stage counts; this port keeps the SignalGenerator example's fixed `6`-stage configuration
