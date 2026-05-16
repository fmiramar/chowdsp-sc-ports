# PhaserFBCh SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `ChowPhaserFeedback` module, renamed to `PhaserFBCh` to follow the `*Ch` naming convention for these ports.

## Layout

- `src/PhaserFBCh.cpp`: server plugin implementation
- `src/dsp/ldr.hpp`: LDR response shaping adapted from the VCV module
- `Classes/PhaserFBCh.sc`: sclang class wrapper
- `HelpSource/Classes/PhaserFBCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `PhaserFBCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the feedback half of the Chow Phaser Rack module:

- LDR-inspired skew mapping from bipolar LFO input
- frequency-warped second-order allpass feedback section
- feedback amount in the original `0..0.95` range
- output saturation with the original `tanh(y / 5) * 5` mapping

This package intentionally does not include the separate modulation-stage module. The LFO input keeps the Rack convention of roughly `-5..+5` units for a bipolar modulation source, so in SuperCollider you will typically scale a normal `-1..+1` LFO by `5`.
