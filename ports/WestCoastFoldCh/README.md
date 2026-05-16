# WestCoastFoldCh SuperCollider Port

Standalone SuperCollider server-plugin package for the chowdsp_utils `WestCoastWavefolder`, exposed as `WestCoastFoldCh` to match the standalone `*Ch` naming convention under `ports/`.

## Layout

- `src/WestCoastFoldCh.cpp`: server plugin implementation
- `Classes/WestCoastFoldCh.sc`: sclang class wrapper
- `HelpSource/Classes/WestCoastFoldCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

The staged extension folder is `WestCoastFoldCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the standalone ADAA Buchla-style folder only. It keeps the original fold approximation and one-sample ADAA latency, but leaves drive and makeup gain to ordinary SC signal arithmetic around the UGen.
