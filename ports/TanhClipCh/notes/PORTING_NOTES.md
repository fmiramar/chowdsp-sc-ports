# TanhClipCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/TanhClipCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
Port target: the chowdsp_utils `ADAATanhClipper`, exposed as:

```supercollider
TanhClipCh.ar(in, mul = 1, add = 0)
```

This scope includes:

1. the original `tanh` transfer curve
2. the original second-order ADAA state update
3. the original one-sample latency

This scope does not yet include:

1. numeric comparison against chowdsp_utils
2. user-facing drive or output parameters beyond SC arithmetic
3. a multichannel internal version; SC multichannel expansion is the intended route
