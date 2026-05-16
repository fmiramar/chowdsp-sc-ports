# TR808OutCh SuperCollider Port

Standalone SuperCollider server-plugin package for the `OutputFilter` block from ChowDSP's `WaveDigitalFilters/TR_808`, renamed to `TR808OutCh` to follow the local `*Ch` naming convention.

## Layout

- `src/TR808OutCh.cpp`: server plugin implementation
- `src/dsp/`: local WDF support plus the adapted TR-808 output filter
- `Classes/TR808OutCh.sc`: sclang class wrapper
- `HelpSource/Classes/TR808OutCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `TR808OutCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the active WaveDigitalFilters output-stage behavior:

- the original WDF output filter circuit
- normalized `volume` and `tone` controls from the source plugin
- audio-rate modulation support for both controls
