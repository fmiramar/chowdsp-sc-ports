# WarpCh SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `Warp` module, renamed to `WarpCh` to follow the `*Ch` naming convention for these ports.

## Layout

- `src/WarpCh.cpp`: server plugin implementation
- `src/dsp/`: local DSP helpers for the nonlinear peaking filter, Newton-Raphson loop solver, and fixed 2x oversampling
- `Classes/WarpCh.sc`: sclang class wrapper
- `HelpSource/Classes/WarpCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `WarpCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the Warp module at the module-control level:

- the original four normalized controls: `cutoff`, `heat`, `width`, and `drive`
- discrete `mode` selection in `0..2`
- fixed 2x oversampling
- nonlinear peaking filter plus delay-free feedback loop
- 16-sample parameter cooking cadence with the original mode-change fade

Unlike the Rack module, this standalone port does not expose the right-click oversampling menu. The oversampling ratio is fixed to the original default of 2x.
