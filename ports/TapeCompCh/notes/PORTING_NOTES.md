# TapeCompCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/TapeCompCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the standalone ChowTape compression stage, exposed as:

```supercollider
TapeCompCh.ar(in, amount = 0, attack = 0.5, release = 0.5, mul = 1, add = 0)
```

This scope includes:

1. the original compression gain computer
2. the original attack and release mappings
3. the original level-detector smoothing behavior
4. the original 32-sample parameter cook cadence

This scope does not yet include:

1. rendered A/B comparisons against the original Rack module
2. numeric comparison against the original Rack DSP path
3. any larger combined `ChowTape` multi-stage wrapper

## Source Mapping
Relevant source files:

- `ChowDSP-VCV/src/ChowTape/compression/ChowTapeCompression.cpp`
- `ChowDSP-VCV/src/shared/LevelDetector.hpp`
- `ChowDSP-VCV/doc/manual.md`

## Architecture Decisions
1. Keep the port self-contained under `ports/TapeCompCh/`.
2. Preserve the original module-control surface instead of exposing cooked times directly.
3. Copy the simple `LevelDetector` locally rather than pulling in the broader shared Rack utility layer.
4. Keep the Rack module's 32-sample parameter update cadence so time-constant changes are not recalculated every sample.
