# PhaserFBCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/PhaserFBCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the feedback module from Chow Phaser, exposed as:

```supercollider
PhaserFBCh.ar(in, lfo = 0, skew = 0, feedback = 0, mul = 1, add = 0)
```

This scope includes:

1. the LDR-style skew shaping applied to the LFO
2. the warped second-order allpass feedback filter
3. the output soft saturation used by the VCV module

This scope does not yet include:

1. the separate Chow Phaser modulation-stage module
2. a combined higher-level phaser wrapper
3. alternative parameter mappings tailored specifically for SuperCollider

## Source Mapping
The current VCV sample path is:

1. normalize the LFO from Rack voltage into `-1..1`
2. map the skewed LFO through the LDR resistance curve
3. compute the analog feedback/allpass coefficients
4. apply the pole-frequency warp and bilinear transform
5. process the input through the biquad state update
6. saturate the output with `tanh(y / 5) * 5`

Relevant source files:

- `ChowDSP-VCV/src/ChowPhaser/ChowPhaserFB.cpp`
- `ChowDSP-VCV/src/ChowPhaser/ldr.hpp`
- `ChowDSP-VCV/src/shared/iir.hpp`
- `ChowDSP-VCV/doc/manual.md`

## Architecture Decisions
1. Keep the port self-contained under `ports/PhaserFBCh/`.
2. Port the feedback module directly instead of approximating it with stock SuperCollider filter UGens.
3. Preserve the Rack parameter ranges for the first pass: `skew` in `-1..1` and `feedback` in `0..0.95`.
4. Allow audio-rate modulation for `in`, `lfo`, `skew`, and `feedback`.
5. Implement the biquad state update directly in the SC unit rather than embedding the original C++ helper type, so the port stays simple and avoids unnecessary construction hazards.
6. Keep this package ABI-matched to the installed `SuperCollider 3.14.1` runtime by building against `../../supercollider-3.14.1`.

## Next Recommended Step
After compile and listening validation:

1. compare a few modulation sweeps against the VCV module
2. decide whether `PhaserFBCh` and `PhaserModCh` should get a higher-level wrapper UGen or class
3. evaluate whether `supernova` support is worth building for these standalone ports
