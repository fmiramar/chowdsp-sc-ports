# SimpleReverbCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/SimpleReverbCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the `SimpleReverb` example plugin from `chowdsp_utils`, exposed as:

```supercollider
SimpleReverbCh.ar(inL, inR, diffusionTime = 100, fdnDelay = 100, fdnT60Low = 500, fdnT60High = 500, modAmount = 0, dryWet = 0.25)
```

This scope includes:

1. the original public parameter surface
2. a stereo 8-line FDN core with low/high decay shaping
3. the original two internal modulation LFO frequencies

This scope does not yet include:

1. the original long `ConvolutionDiffuser` implementation
2. numeric comparison against the original chowdsp_utils example
3. rendered A/B listening comparison against the original plugin

## Architecture Decisions
1. Keep the port self-contained under `ports/SimpleReverbCh/`.
2. Replace the original `ConvolutionDiffuser` with a deterministic stereo diffuser chain built from delay/mix stages that are practical inside an SC UGen.
3. Use RT-allocated dynamic delay buffers instead of embedding very large static delay storage in the unit.
4. Keep the example's stereo duplication pattern into an 8-line FDN and average the even/odd returns back to left/right.
