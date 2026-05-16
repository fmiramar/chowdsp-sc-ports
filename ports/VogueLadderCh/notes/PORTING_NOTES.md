# VogueLadderCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/VogueLadderCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
Port target: the public BYOD ladder filter module, exposed as:

```supercollider
VogueLadderCh.ar(in, hpCutoff = 0, hpResonance = 0, lpCutoff = 1, lpResonance = 0, drive = 0.5, oscillating = 0, stereo = 0, mul = 1, add = 0)
```

This scope includes:

1. the resonant 4-pole ladder high-pass section
2. the resonant 4-pole ladder low-pass section
3. the source module's normalized control mappings
4. the source drive gain and saturation blend
5. the oscillating/non-oscillating resonance cap behavior

## Source Mapping
Relevant source files:

- `BYOD/src/processors/tone/ladder_filter/LadderFilterProcessor.cpp`
- `BYOD/src/processors/tone/ladder_filter/HighPassLadder.*`
- `BYOD/src/processors/tone/ladder_filter/LowPassLadder.*`
- `BYOD/src/processors/tone/ladder_filter/HighPassOnePole.*`
- `BYOD/src/processors/tone/ladder_filter/LowPassOnePole.*`
- `BYOD/src/processors/tone/ladder_filter/utility.h`

## Architecture Decisions
1. Keep the port self-contained under `ports/VogueLadderCh/`.
2. Port the DSP core directly rather than carrying over BYOD's `BaseProcessor` and parameter framework.
3. Preserve the BYOD normalized control space on the first pass.
4. Expose the oscillation mode as a simple scalar input instead of a GUI choice.
5. Keep the SC interface audio-rate only for now.

## Compatibility Note
The installed SuperCollider app on this machine is `3.14.1` (`426edf6`) and expects plugin API `3`.

For local testing, use:

- `SC_PATH=../../supercollider-3.14.1`
- staged output: `ports/VogueLadderCh/build-3.14.1/stage/VogueLadderCh/`

## Next Recommended Step
After compile and listening validation:

1. compare the filter sweep behavior against BYOD for a few parameter sets
2. decide whether a second interface with physical cutoff/resonance units is worth adding
3. decide whether the stereo interface should become a true stereo-input UGen instead of dual-mono output
