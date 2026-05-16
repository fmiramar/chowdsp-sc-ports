# AutoWahCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/AutoWahCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the `AutoWah` example plugin from `chowdsp_utils`, exposed as:

```supercollider
AutoWahCh.ar(in, freq = 500, q = 5, gainDB = 0, attackMs = 1, releaseMs = 50, freqMod = 0.5, mul = 1, add = 0)
```

This scope includes:

1. the original level-detector parameter units
2. the original peaking-filter control units
3. the original sweep law `baseFreq + baseFreq * (9 * freqMod) * level`
4. the scalar equivalent of the chowdsp modulated peaking-filter wrapper

This scope does not yet include:

1. rendered A/B comparisons against the original JUCE example plugin
2. numeric comparison against the original chowdsp filter wrapper
3. a stereo-linked internal version; SC multichannel expansion is the intended route for now

## Source Mapping
Relevant source files:

- `chowdsp_utils/examples/AutoWah/AutoWahPlugin.cpp`
- `chowdsp_utils/modules/dsp/chowdsp_dsp_utils/Processors/chowdsp_LevelDetector.h`
- `chowdsp_utils/modules/dsp/chowdsp_dsp_utils/Processors/chowdsp_LevelDetector.cpp`
- `chowdsp_utils/modules/dsp/chowdsp_filters/LowerOrderFilters/chowdsp_ModFilterWrapper.h`
- `chowdsp_utils/modules/dsp/chowdsp_filters/LowerOrderFilters/chowdsp_ModFilterWrapper.cpp`
- `chowdsp_utils/modules/dsp/chowdsp_filters/LowerOrderFilters/chowdsp_SecondOrderFilters.h`
- `chowdsp_utils/modules/dsp/chowdsp_filters/Utils/chowdsp_CoefficientCalculators.h`
- `chowdsp_utils/modules/dsp/chowdsp_filters/Utils/chowdsp_ConformalMaps.h`

## Architecture Decisions
1. Keep the port self-contained under `ports/AutoWahCh/`.
2. Port only the detector and modulated peaking-filter behavior needed by the example, not the wider chowdsp module stack.
3. Keep the original parameter units instead of remapping everything to normalized controls.
4. Clamp the public controls to the original plugin ranges before coefficient calculation, then clamp the modulated center frequency to a safe fraction of Nyquist.
5. Add only minimal non-finite recovery around the SC integration.
