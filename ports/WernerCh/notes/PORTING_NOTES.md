# WernerCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/WernerCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the active Werner Filter DSP core, exposed as:

```supercollider
WernerCh.ar(in, freq = 632.4555, feedback = 0.5, damping = 0.5, drive = 0.1, mul = 1, add = 0)
```

This scope includes:

1. the generalized nonlinear SVF core
2. fixed 2x oversampling
3. the module's 16-sample parameter cooking cadence

This scope does not yet include:

1. the Rack module's CV attenuator model
2. selectable oversampling ratios
3. a higher-level wrapper exposing alternative parameterizations

## Source Mapping
The current VCV sample path is:

1. cook filter parameters from frequency, feedback, damping, and drive
2. upsample the input by 2x
3. process each oversampled sample through the generalized SVF
4. downsample the result

Relevant source files:

- `ChowDSP-VCV/src/Werner/Werner.cpp`
- `ChowDSP-VCV/src/Werner/GenSVF.hpp`
- `ChowDSP-VCV/src/Werner/GenSVF.cpp`
- `ChowDSP-VCV/src/shared/oversampling.hpp`
- `ChowDSP-VCV/src/shared/VariableOversampling.hpp`
- `ChowDSP-VCV/doc/manual.md`

## Architecture Decisions
1. Keep the port self-contained under `ports/WernerCh/`.
2. Port the active DSP path directly instead of approximating it with stock SuperCollider filters.
3. Fix oversampling at 2x for the first version, because that is the module default and keeps the interface simple.
4. Keep the original 16-sample parameter cooking cadence instead of recalculating the state-space coefficients every sample.
5. Expose cooked DSP parameters directly in the SC interface: `freq` in Hz and `drive` as the internal nonlinear gain, rather than preserving the Rack knob-plus-attenuator UI mapping.
6. Implement the 4x4 matrix math locally instead of depending on Eigen in the plugin build.
7. Keep this package ABI-matched to the installed `SuperCollider 3.14.1` runtime by building against `../../supercollider-3.14.1`.

## Next Recommended Step
After compile and listening validation:

1. compare a few sweeps against the VCV module at matched cooked parameters
2. decide whether a normalized Rack-style wrapper should be added in sclang
3. evaluate whether exposing oversampling choice is worth the extra interface complexity
