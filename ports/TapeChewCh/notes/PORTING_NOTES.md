# TapeChewCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/TapeChewCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the standalone ChowTape chew stage, exposed as:

```supercollider
TapeChewCh.ar(in, depth = 0, frequency = 0, variance = 0, mul = 1, add = 0)
```

This scope includes:

1. the original normalized control surface
2. dropout shaping and smoothed lowpass filtering
3. randomized wet/dry chew timing and chew-depth variation
4. the original 64-sample parameter cook cadence

This scope does not yet include:

1. rendered A/B comparisons against the original Rack module
2. numeric comparison against the original Rack DSP path
3. any larger combined `ChowTape` multi-stage wrapper

## Source Mapping
Relevant source files:

- `ChowDSP-VCV/src/ChowTape/degrade/ChowTapeChew.cpp`
- `ChowDSP-VCV/src/ChowTape/degrade/Dropout.h`
- `ChowDSP-VCV/src/ChowTape/degrade/DegradeFilter.h`
- `ChowDSP-VCV/src/shared/SmoothedValue.h`
- `ChowDSP-VCV/doc/manual.md`

## Architecture Decisions
1. Keep the port self-contained under `ports/TapeChewCh/`.
2. Replace the Rack/JUCE smoothing helpers with small local equivalents that only implement the linear and multiplicative behaviors used by this stage.
3. Replace Rack global randomness with local deterministic xorshift RNGs so the standalone port has no Rack runtime dependency.
4. Guard the randomized wet/dry duration range so the standalone port cannot hit a zero-width modulo path.
5. Keep the Rack parameter mappings and 64-sample cook cadence instead of exposing cooked chew timings directly.
