# TapeCh SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `ChowTape` module, exposed as `TapeCh` to match the standalone `*Ch` naming convention under `ports/`.

## Layout

- `src/TapeCh.cpp`: server plugin implementation
- `src/dsp/`: local hysteresis and oversampling helpers
- `Classes/TapeCh.sc`: sclang class wrapper
- `HelpSource/Classes/TapeCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `TapeCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the standalone ChowTape stage at the original module-control level:

- normalized `bias`, `saturation`, and `drive`
- original hysteresis solver path
- Rack default fixed `4x` oversampling
- original per-sample parameter cooking

The current Rack source configures a DC blocker but does not run the signal through it. This standalone port preserves that source behavior rather than silently changing the nonlinear stage.
