# FDNCh SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `ChowFDN` module, exposed as `FDNCh` to match the short `*Ch` naming used by the other standalone ports.

## Layout

- `src/FDNCh.cpp`: server plugin implementation
- `src/dsp/`: local DSP helpers for dynamic delay lines, delay scheduling, damping shelves, and the deterministic mix matrix
- `Classes/FDNCh.sc`: sclang class wrapper
- `HelpSource/Classes/FDNCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `FDNCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the ChowFDN module at the original module-control level:

- exponential pre-delay control
- room `size`, `t60High`, `t60Low`, `numDelays`, and `mix`
- original delay-length schedule and damping shelf model
- SC-safe dynamic delay buffers sized to the actual delay ranges
- deterministic orthonormal mix matrix for reproducible behavior

Unlike the Rack module, this standalone port does not use a time-seeded random mixing matrix. The matrix is deterministic so repeated runs are reproducible and do not rely on the Rack-side matrix exponential utility.
