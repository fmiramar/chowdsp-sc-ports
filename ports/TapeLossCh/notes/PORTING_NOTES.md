# TapeLossCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/TapeLossCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the standalone ChowTape loss stage, exposed as:

```supercollider
TapeLossCh.ar(in, gap = 0.5, thickness = 0.5, spacing = 0.5, speed = 0.5, mul = 1, add = 0)
```

This scope includes:

1. the original normalized control surface
2. the sample-rate-scaled FIR response rebuild
3. the head-bump peak filter
4. the original 128-sample parameter cook cadence

This scope does not yet include:

1. rendered A/B comparisons against the original Rack module
2. numeric comparison against the original Rack DSP path
3. any larger combined `ChowTape` multi-stage wrapper

## Source Mapping
Relevant source files:

- `ChowDSP-VCV/src/ChowTape/loss/ChowTapeLoss.cpp`
- `ChowDSP-VCV/src/ChowTape/loss/FIRFilter.h`
- `ChowDSP-VCV/src/shared/iir.hpp`
- `ChowDSP-VCV/doc/manual.md`

## Architecture Decisions
1. Keep the port self-contained under `ports/TapeLossCh/`.
2. Use a fixed-size local FIR helper instead of dynamically resizing buffers at runtime.
3. Preserve the Rack parameter mappings rather than exposing the cooked physical values directly.
4. Clear the FIR work buffers before each rebuild. From the source, this appears to be an unintended omission in the Rack module when the effective FIR order is odd.
