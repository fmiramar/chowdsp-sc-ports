# DistortCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/DistortCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the active ChowDer core from the VCV module, exposed as:

```supercollider
DistortCh.ar(in, bass = 0, treble = 0, drive = 0.5, bias = 0, mul = 1, add = 0)
```

This scope includes:

1. the normalized shelf filter used in the current module
2. the diode clipping stage
3. fixed 2x oversampling

This scope does not yet include:

1. selectable oversampling ratios
2. the Rack-side DC blocker
3. a higher-level wrapper for alternative parameterizations

## Source Mapping
The current VCV sample path is:

1. normalized bass/treble are mapped to shelf gains
2. input is filtered, driven, and biased
3. signal is clipped by the diode stage
4. result is downsampled

Relevant source files:

- `ChowDSP-VCV/src/ChowDer/ChowDer.cpp`
- `ChowDSP-VCV/src/ChowDer/ClippingStage.hpp`
- `ChowDSP-VCV/src/shared/shelf_filter.hpp`
- `ChowDSP-VCV/src/shared/oversampling.hpp`

## Architecture Decisions
1. Keep the port self-contained under `ports/DistortCh/`.
2. Port the currently active ChowDer path, not the older unused `BaxandallEQ` WDF code.
3. Fix oversampling at 2x for the first version, because that matches the module default and keeps the SC interface simple.
4. Omit the internal DC blocker and document the expectation that SC users add `LeakDC` or `HPF` themselves when needed.
5. Preserve the VCV control mapping ranges for the first pass instead of inventing a new parameter model.
6. For embedded C++ DSP members in SC units, use placement new with default-initialization (`new (unit) DistortCh;`), not value-initialization (`new (unit) DistortCh();`), so the server-owned `Unit` base is not clobbered.

## Compatibility Note
The installed SuperCollider app on this machine is `3.14.1` (`426edf6`) and expects plugin API `3`.

For local testing, use:

- `SC_PATH=../../supercollider-3.14.1`
- staged output: `ports/DistortCh/build-3.14.1/stage/DistortCh/`

## Next Recommended Step
After compile and listening validation:

1. add a small runtime smoke test
2. compare the distortion transfer against the VCV module for a few parameter sets
3. expose oversampling choice if the fixed 2x version proves stable
