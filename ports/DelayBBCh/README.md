# DelayBBCh

Standalone SuperCollider server-plugin package for a mono bucket-brigade delay, exposed as `DelayBBCh`.

## Layout

- `src/DelayBBCh.cpp`: server plugin implementation
- `src/dsp/`: local BBD delay helpers and the small DC-blocking biquad
- `Classes/DelayBBCh.sc`: sclang wrapper
- `HelpSource/Classes/DelayBBCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

The package defaults to `Release` if no CMake build type is specified, because the BBD inner loop is significantly more expensive in unoptimized builds.

## Current Status

This first draft exposes the validated scalar BBD line from `ChorusCh` as a direct mono delay effect with:

- fixed `4096`-stage BBD core
- `delayTime`, `filterFreq`, `feedback`, `driveDB`, and `mix`
- bipolar feedback with DC blocking
- input saturation before the BBD line

The internal BBD model is the same Juno-style delay/filter-bank path already used in the standalone chorus port, not a generic interpolated digital delay line.
