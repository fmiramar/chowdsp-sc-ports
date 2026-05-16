# DelayBBCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/DelayBBCh/`

It is intentionally not being added to `sc3-plugins`. The naming convention remains `*Ch`.

## Scope Chosen
This package turns the validated BBD core already ported for `ChorusCh` into a direct effect UGen:

```supercollider
DelayBBCh.ar(in, delayTime = 0.08, filterFreq = 10000, feedback = 0.2, driveDB = 0, mix = 0.5)
```

This scope includes:

1. a fixed `4096`-stage BBD line
2. the same four-branch Juno-style input/output BBD filter banks used in `ChorusCh`
3. bipolar feedback with DC blocking
4. input saturation before the delay line
5. direct dry/wet mixing

This scope does not include:

1. alternate stage-count variants
2. tempo sync or tap tempo
3. stereo wrappers
4. any claim of being a verbatim port of a separate plugin UI from `WaveDigitalFilters`

## Architecture Decisions
1. Keep the package self-contained instead of refactoring `ChorusCh` into shared DSP headers first.
2. Reuse the already-validated scalar BBD model from the chorus port rather than introducing a second independent BBD implementation.
3. Keep the delay mono per instance and rely on normal SC multichannel expansion for stereo use.
4. Make `feedback` bipolar because that is more useful for a standalone delay effect than the chorus module's strictly positive feedback law.
5. Keep `delayTime` in seconds rather than exposing BBD clock rate directly.

## Runtime Note
Shorter delay times increase CPU because the BBD clock loop runs more internal half-steps per audio sample. This is expected behavior for this model.
