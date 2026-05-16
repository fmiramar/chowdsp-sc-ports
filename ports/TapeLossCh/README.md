# TapeLossCh SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `ChowTapeLoss` module, exposed as `TapeLossCh` to match the standalone `*Ch` naming convention under `ports/`.

## Layout

- `src/TapeLossCh.cpp`: server plugin implementation
- `src/dsp/fir_filter.hpp`: fixed-size local FIR helper
- `src/dsp/biquad.hpp`: local peak-filter helper for the head-bump stage
- `Classes/TapeLossCh.sc`: sclang class wrapper
- `HelpSource/Classes/TapeLossCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `TapeLossCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the standalone ChowTape loss stage at the original module-control level:

- normalized `gap`, `thickness`, `spacing`, and `speed`
- sample-rate-scaled FIR reconstruction
- head-bump peak filter
- original 128-sample parameter cook cadence

The coefficient rebuild logic is intentionally kept close to the Rack source, but the coefficient work buffers are cleared before each rebuild so prior parameter states do not leak into later FIR responses when the effective order is odd.
