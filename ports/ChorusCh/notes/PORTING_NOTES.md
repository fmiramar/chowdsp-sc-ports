# ChorusCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/ChorusCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the full ChowChorus module control surface, exposed as:

```supercollider
ChorusCh.ar(in, rate = 0.5, depth = 0.5, feedback = 0.0, mix = 0.5)
```

This scope includes:

1. the original mono-input, stereo-output topology
2. the original exponential rate mapping for the slow and fast LFO pairs
3. the original feedback law and dry/wet mix
4. the original anti-aliasing and DC-blocking post filters
5. the original 64-sample parameter-cook cadence

This scope does not yet include:

1. rendered A/B comparisons against the original Rack module
2. profiling against the Rack SSE implementation
3. optional mono-downmixed or widened alternative wrappers

## Source Mapping
The current VCV sample path is:

1. map `rate` to the slow and fast LFO frequencies and `feedback` to the feedback gain
2. compute per-sample slow and fast delay offsets from `depth`
3. run two BBD-style delay lines per output channel
4. lowpass the summed delay output, feed the filtered signal back through a DC blocker, and apply the dry/wet mix

Relevant source files:

- `ChowDSP-VCV/src/ChowChorus/ChowChorus.cpp`
- `ChowDSP-VCV/src/ChowChorus/BBDDelayLine.h`
- `ChowDSP-VCV/src/ChowChorus/BBDDelayLine.cpp`
- `ChowDSP-VCV/src/ChowChorus/BBDFilterBank.h`
- `ChowDSP-VCV/src/shared/SineWave.h`
- `ChowDSP-VCV/src/shared/SineWave.cpp`
- `ChowDSP-VCV/src/shared/iir.hpp`
- `ChowDSP-VCV/doc/manual.md`

## Architecture Decisions
1. Keep the port self-contained under `ports/ChorusCh/`.
2. Preserve the original mono-input, stereo-output topology instead of collapsing the effect to mono.
3. Reimplement the four-branch BBD filter-bank math locally with scalar complex arithmetic so the port does not depend on Rack, JUCE, or Surge headers.
4. Keep the control-surface abstraction rather than exposing the four internal LFOs or per-delay times directly.
5. Add explicit subnormal flushing in the BBD/filter feedback path because the scalar port can otherwise hit denormal slowdowns once the chorus tail decays.
6. Default this package to `Release` when no build type is specified, because the scalar BBD path is too expensive to ship unoptimized.
7. Build this package against `../../supercollider-3.14.1` to stay ABI-matched with the installed runtime.

## Next Recommended Step
After compile and listening validation:

1. render swept and impulsive test material against the original Rack module
2. decide whether the scalar filter-bank implementation needs an SSE fast path
3. evaluate whether a mono helper wrapper or a wider stereo wrapper would be useful in sclang
