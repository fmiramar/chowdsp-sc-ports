# TapeCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/TapeCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the standalone ChowTape nonlinear stage, exposed as:

```supercollider
TapeCh.ar(in, bias = 0.5, saturation = 0.5, drive = 0.5, mul = 1, add = 0)
```

This scope includes:

1. the original normalized control surface
2. the original hysteresis solver with `NR4`
3. the Rack default `4x` oversampling path
4. the original per-sample parameter cooking

This scope does not yet include:

1. rendered A/B comparisons against the original Rack module
2. numeric comparison against the original Rack DSP path
3. the Rack context-menu oversampling options beyond the default `4x` setting

## Source Mapping
Relevant source files:

- `ChowDSP-VCV/src/ChowTape/tape/ChowTape.cpp`
- `ChowDSP-VCV/src/ChowTape/tape/HysteresisProcessing.hpp`
- `ChowDSP-VCV/src/ChowTape/tape/HysteresisProcessing.cpp`
- `ChowDSP-VCV/src/shared/VariableOversampling.hpp`
- `ChowDSP-VCV/src/shared/oversampling.hpp`
- `ChowDSP-VCV/doc/manual.md`

## Architecture Decisions
1. Keep the port self-contained under `ports/TapeCh/`.
2. Fix oversampling at `4x`, which is the Rack module's constructor default, instead of porting the UI-only context menu.
3. Port the hysteresis solver math directly and keep `NR4` as the active solver.
4. Keep the module's per-sample parameter cooking instead of adding a coarser SC-side recook cadence.
5. Preserve the current Rack source behavior where the DC blocker is configured but not applied to the audio output.
6. Add only a minimal non-finite safety reset around the SC integration so bad state cannot poison the server indefinitely.
