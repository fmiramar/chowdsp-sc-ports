# PhaserModCh SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `ChowPhaserMod` module, renamed to `PhaserModCh` to follow the `*Ch` naming convention for these ports.

## Layout

- `src/PhaserModCh.cpp`: server plugin implementation
- `src/dsp/ldr.hpp`: LDR response shaping adapted from the VCV module
- `Classes/PhaserModCh.sc`: sclang class wrapper
- `HelpSource/Classes/PhaserModCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `PhaserModCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the modulation half of the Chow Phaser Rack module:

- LDR-inspired skew mapping from bipolar LFO input
- variable number of modulated allpass stages
- fractional stage interpolation
- wet/dry modulation mix

This package intentionally does not include the separate feedback module. The LFO input keeps the Rack convention of roughly `-5..+5` units for a bipolar modulation source, so in SuperCollider you will typically scale a normal `-1..+1` LFO by `5`.
