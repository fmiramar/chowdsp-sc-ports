# TapeCompCh SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `ChowTapeCompression` module, exposed as `TapeCompCh` to match the standalone `*Ch` naming convention under `ports/`.

## Layout

- `src/TapeCompCh.cpp`: server plugin implementation
- `src/dsp/level_detector.hpp`: local level-detector helper adapted from the VCV module
- `Classes/TapeCompCh.sc`: sclang class wrapper
- `HelpSource/Classes/TapeCompCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `TapeCompCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the standalone ChowTape compression stage at the original module-control level:

- `amount` in dB
- normalized `attack` and `release` controls with the original exponential mappings
- original level-detector smoothing and gain computer
- original 32-sample parameter cook cadence

Unlike the Rack module, this port does not expose any GUI-specific metadata. It is a direct mono-input, mono-output DSP port intended to behave like the Rack stage inside an SC signal chain.
