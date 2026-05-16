# TapeDegradeCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/TapeDegradeCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the standalone ChowTape degrade stage, exposed as:

```supercollider
TapeDegradeCh.ar(in, depth = 0, amount = 0, variance = 0, envelope = 0, mul = 1, add = 0)
```

This scope includes:

1. the original normalized control surface
2. degrade-noise injection
3. smoothed lowpass and smoothed gain targets
4. envelope-followed noise
5. the original 64-sample parameter cook cadence

This scope does not yet include:

1. rendered A/B comparisons against the original Rack module
2. numeric comparison against the original Rack DSP path
3. any larger combined `ChowTape` multi-stage wrapper

## Source Mapping
Relevant source files:

- `ChowDSP-VCV/src/ChowTape/degrade/ChowTapeDegrade.cpp`
- `ChowDSP-VCV/src/ChowTape/degrade/DegradeFilter.h`
- `ChowDSP-VCV/src/ChowTape/degrade/DegradeNoise.h`
- `ChowDSP-VCV/src/shared/LevelDetector.hpp`
- `ChowDSP-VCV/src/shared/SmoothedValue.h`
- `ChowDSP-VCV/doc/manual.md`

## Architecture Decisions
1. Keep the port self-contained under `ports/TapeDegradeCh/`.
2. Replace the Rack/JUCE smoothing helpers with small local equivalents that only implement the linear and multiplicative behaviors used by this stage.
3. Replace Rack global randomness with local deterministic xorshift RNGs so the standalone port has no Rack runtime dependency.
4. Keep the Rack parameter mappings and 64-sample cook cadence instead of exposing cooked targets directly.
