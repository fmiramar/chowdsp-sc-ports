# ModalSpringCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/ModalSpringCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the `ModalSpringReverb` example plugin from `chowdsp_utils`, exposed as:

```supercollider
ModalSpringCh.ar(in, pitch = 0, decay = 0.5, mix = 1, modModes = 0, modFreq = 1, modDepth = 0, mul = 1, add = 0)
```

This scope includes:

1. the original measured spring mode tables
2. the original pitch and decay mappings
3. the original dry/wet mix
4. the original per-sample modulation of the lowest `0..200` modes

This scope does not yet include:

1. rendered A/B comparisons against the original JUCE example plugin
2. the original xsimd vectorized bank
3. stereo input summing or multi-output duplication; SC multichannel expansion is the intended route for now

## Source Mapping
Relevant source files:

- `chowdsp_utils/examples/ModalSpringReverb/ModalReverbPlugin.cpp`
- `chowdsp_utils/examples/ModalSpringReverb/ModalReverbPlugin.h`
- `chowdsp_utils/examples/ModalSpringReverb/ModeParams.h`
- `chowdsp_utils/modules/dsp/chowdsp_modal_dsp/ModalFilters/chowdsp_ModalFilter.h`
- `chowdsp_utils/modules/dsp/chowdsp_modal_dsp/ModalFilters/chowdsp_ModalFilterBank.h`
- `chowdsp_utils/modules/dsp/chowdsp_modal_dsp/ModalFilters/chowdsp_ModalFilterBank.cpp`

## Architecture Decisions
1. Keep the port self-contained under `ports/ModalSpringCh/`.
2. Reuse the original measured `ModeParams.h` table at build time through a tiny compatibility shim instead of copying the large arrays into the port.
3. Replace the xsimd modal bank with a scalar bank of Max Mathews phasor filters.
4. Keep the original parameter laws, including the example plugin's `20`-mode quantization for `modModes`.
5. Keep the original very large wet attenuation after the bank render so the normalized mode amplitudes remain stable inside the standalone port.
