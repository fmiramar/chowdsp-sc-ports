# TanhClipCh SuperCollider Port

Standalone SuperCollider server-plugin package for the chowdsp_utils `ADAATanhClipper`, exposed as `TanhClipCh` to match the standalone `*Ch` naming convention under `ports/`.

## Layout

- `src/TanhClipCh.cpp`: server plugin implementation
- `Classes/TanhClipCh.sc`: sclang class wrapper
- `HelpSource/Classes/TanhClipCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

The staged extension folder is `TanhClipCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the standalone ADAA tanh clipper only. It keeps the original transfer function and one-sample ADAA latency, but does not add any extra drive or output controls beyond normal SC signal arithmetic.
