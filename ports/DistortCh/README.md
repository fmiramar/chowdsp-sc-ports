# DistortCh SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `ChowDer` module, renamed to `DistortCh` to follow the `*Ch` naming convention for these ports.

## Layout

- `src/DistortCh.cpp`: server plugin implementation
- `src/dsp/`: local DSP support copied or adapted from the VCV module
- `Classes/DistortCh.sc`: sclang class wrapper
- `HelpSource/Classes/DistortCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `DistortCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the active ChowDer core that is actually used in the VCV module:

- normalized bass/treble shelf filter
- normalized drive and bias mapping
- diode clipping stage
- fixed 2x oversampling

It does not yet expose the VCV module's selectable oversampling menu, and it omits the internal DC blocker so SuperCollider users can choose `LeakDC` or `HPF` explicitly.
