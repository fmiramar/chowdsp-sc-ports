# WarpCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/WarpCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the full Warp module control surface, exposed as:

```supercollider
WarpCh.ar(in, cutoff = 0.5, heat = 0.5, width = 0.5, drive = 0.0, mode = 0, mul = 1, add = 0)
```

This scope includes:

1. the original four normalized control inputs
2. the discrete `mode` selector with the original three mapping sets
3. fixed 2x oversampling
4. the original 16-sample parameter cooking cadence
5. the short fade-in after mode changes

This scope does not yet include:

1. user-selectable oversampling ratios
2. a second UGen exposing the internal peaking-filter coefficients directly
3. rendered A/B comparisons against the original Rack module

## Source Mapping
The current VCV sample path is:

1. map the module controls into the internal Warp filter parameters
2. cook the peaking-filter coefficients and feedback-loop drive values every 16 samples
3. upsample the input by 2x
4. process each oversampled sample through the delay-free loop and nonlinear peaking filter
5. downsample and apply the output gain with the original fade after mode changes

Relevant source files:

- `ChowDSP-VCV/src/Warp/Warp.cpp`
- `ChowDSP-VCV/src/Warp/Warp.hpp`
- `ChowDSP-VCV/src/Warp/WarpFilter.cpp`
- `ChowDSP-VCV/src/Warp/WarpFilter.hpp`
- `ChowDSP-VCV/src/Warp/Mappings.hpp`
- `ChowDSP-VCV/src/Warp/NewtonRaphson.hpp`
- `ChowDSP-VCV/src/shared/nl_biquad.hpp`
- `ChowDSP-VCV/src/shared/oversampling.hpp`
- `ChowDSP-VCV/doc/manual.md`

## Architecture Decisions
1. Keep the port self-contained under `ports/WarpCh/`.
2. Preserve the module-level control interface instead of exposing only the internal WarpFilter coefficients.
3. Fix oversampling at 2x for the first version, because that is the module default and keeps the interface simple.
4. Implement the parameter mappings locally instead of porting the Rack-side `ParamMap` helper.
5. Keep the original 16-sample recook cadence and the 2048-sample fade behavior after mode changes.
6. Keep this package ABI-matched to the installed `SuperCollider 3.14.1` runtime by building against `../../supercollider-3.14.1`.

## Next Recommended Step
After compile and listening validation:

1. render matched sweeps against the original Rack module for each mode
2. decide whether a lower-level `WarpCoreCh` UGen exposing internal coefficients would be useful
3. evaluate whether selectable oversampling is worth the interface and testing cost
