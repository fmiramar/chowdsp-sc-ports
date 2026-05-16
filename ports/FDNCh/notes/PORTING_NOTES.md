# FDNCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/FDNCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the full ChowFDN module control surface, exposed as:

```supercollider
FDNCh.ar(in, preDelay = 0.5, size = 0.5, t60High = 0.5, t60Low = 1.0, numDelays = 4, mix = 1, mul = 1, add = 0)
```

This scope includes:

1. the original six module controls
2. the original delay-length schedule
3. the original first-order damping shelf model
4. truncation of `numDelays` to integer values as in the Rack float-to-int cast

This scope does not yet include:

1. the Rack module's time-seeded random orthonormal matrix generation
2. rendered A/B comparisons against the original Rack module
3. a multichannel version exposing individual delay-line outputs

## Source Mapping
The current VCV sample path is:

1. map the pre-delay knob exponentially to milliseconds and process the input through a fractional pre-delay line
2. update the active FDN delay lengths and damping shelves when `size`, `t60High`, `t60Low`, or `numDelays` change
3. read the active FDN taps
4. mix them through the module's orthonormal matrix, add the input, filter with the damping shelves, and feed them back into the delay lines
5. apply the dry/wet mix

Relevant source files:

- `ChowDSP-VCV/src/ChowFDN/ChowFDN.cpp`
- `ChowDSP-VCV/src/ChowFDN/fdn.cpp`
- `ChowDSP-VCV/src/ChowFDN/fdn.hpp`
- `ChowDSP-VCV/src/ChowFDN/mixing_matrix_utils.hpp`
- `ChowDSP-VCV/src/shared/delay_line.hpp`
- `ChowDSP-VCV/src/shared/delay_line.cpp`
- `ChowDSP-VCV/src/shared/delay_utils.hpp`
- `ChowDSP-VCV/src/shared/shelf_filter.hpp`
- `ChowDSP-VCV/doc/manual.md`

## Architecture Decisions
1. Keep the port self-contained under `ports/FDNCh/`.
2. Preserve the original module-level control interface instead of exposing a lower-level FDN core UGen first.
3. Replace the Rack `DelayLine` storage with SC-safe dynamic RT-allocated buffers sized to the actual delay ranges, because the Rack version embeds a very large fixed history buffer per delay line.
4. Use a deterministic 16x16 orthonormal Walsh-Hadamard-style matrix instead of the Rack module's time-seeded random orthonormal matrix generation, so behavior is reproducible and the port does not depend on `r8lib`.
5. Keep this package ABI-matched to the installed `SuperCollider 3.14.1` runtime by building against `../../supercollider-3.14.1`.

## Next Recommended Step
After compile and listening validation:

1. render impulse and noise tests against the original Rack module to compare decay envelopes and coloration
2. decide whether a randomizable matrix option is worth adding in sclang
3. evaluate whether a stereo or multi-out wrapper is worth adding around the mono core
