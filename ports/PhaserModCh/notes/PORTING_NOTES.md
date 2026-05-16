# PhaserModCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/PhaserModCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the modulation module from Chow Phaser, exposed as:

```supercollider
PhaserModCh.ar(in, lfo = 0, skew = 0, mod = 0, stages = 8, mul = 1, add = 0)
```

This scope includes:

1. the LDR-style skew shaping applied to the LFO
2. the modulated allpass cascade
3. the fractional-stage interpolation from the VCV module
4. the wet/dry modulation mix

This scope does not yet include:

1. the separate Chow Phaser feedback module
2. a combined higher-level phaser wrapper
3. alternative parameter mappings tailored specifically for SuperCollider

## Source Mapping
The current VCV sample path is:

1. normalize the LFO from Rack voltage into `-1..1`
2. map the skewed LFO through the LDR resistance curve
3. compute allpass coefficients from the resistance value
4. process the input through the selected number of allpass stages
5. mix the modulated output with the dry input

Relevant source files:

- `ChowDSP-VCV/src/ChowPhaser/ChowPhaserMod.cpp`
- `ChowDSP-VCV/src/ChowPhaser/ldr.hpp`
- `ChowDSP-VCV/doc/manual.md`

## Architecture Decisions
1. Keep the port self-contained under `ports/PhaserModCh/`.
2. Port the modulation module directly instead of approximating it with stock SuperCollider allpass UGens.
3. Preserve the Rack parameter ranges for the first pass: `skew` in `-1..1`, `mod` in `0..1`, and `stages` in `1..50`.
4. Allow audio-rate modulation for `in`, `lfo`, `skew`, `mod`, and `stages`.
5. Keep the VCV fractional-stage behavior exactly, including the unconditional extra-stage state advance that occurs inside the interpolation expression.
6. Keep this package ABI-matched to the installed `SuperCollider 3.14.1` runtime by building against `../../supercollider-3.14.1`.

## Next Recommended Step
After compile and listening validation:

1. port the feedback half as a separate `*Ch` module if needed
2. compare modulation sweeps against the VCV module for a few stage counts
3. decide whether a higher-level phaser wrapper should combine the modulation and feedback halves
