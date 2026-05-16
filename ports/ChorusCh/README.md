# ChorusCh SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `ChowChorus` module, exposed as `ChorusCh` to match the standalone `*Ch` naming convention used under `ports/`.

## Layout

- `src/ChorusCh.cpp`: server plugin implementation
- `src/dsp/`: local DSP helpers for the BBD delay path, filter bank, biquads, and LFOs
- `Classes/ChorusCh.sc`: sclang multi-output wrapper
- `HelpSource/Classes/ChorusCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DCMAKE_BUILD_TYPE=Release -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `ChorusCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the ChowChorus module at the module-control level:

- mono input with the original stereo output topology
- normalized `rate`, `depth`, `feedback`, and `mix` controls
- the original two-delay-per-side modulation structure and channel phase offsets
- the original anti-aliasing lowpass and DC-blocked feedback path
- the original 64-sample parameter-cook cadence
- explicit subnormal flushing in the BBD, biquad, and feedback states to stop CPU blow-ups in quiet tails

Unlike the Rack module, this standalone port uses a local scalar complex implementation of the four-branch BBD filter bank instead of the Rack/Surge SSE wrapper. That means optimized builds matter a lot more here, so the package now defaults to `Release` when no build type is specified. The signal flow and control mapping are intended to match the original module, but no rendered A/B comparison has been completed yet.
