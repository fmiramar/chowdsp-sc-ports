# WernerCh SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `Werner` module, renamed to `WernerCh` to follow the `*Ch` naming convention for these ports.

## Layout

- `src/WernerCh.cpp`: server plugin implementation
- `src/dsp/`: local DSP support for the generalized SVF and fixed 2x oversampling
- `Classes/WernerCh.sc`: sclang class wrapper
- `HelpSource/Classes/WernerCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `WernerCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the active Werner Filter DSP path:

- generalized nonlinear SVF core
- fixed 2x oversampling
- 16-sample parameter cooking cadence, matching the VCV module
- direct exposure of the cooked DSP parameters
- local 4x4 matrix implementation numerically checked against the original Eigen SVF via `ports/tests/dsp_sanity.cpp`

Unlike the Rack module, this UGen does not expose CV attenuator knobs or selectable oversampling ratios. It uses `freq` in Hz, `feedback` in `0..1`, `damping` in `0.25..1.25`, and `drive` as the internal nonlinear gain in approximately `0.1..10`.
