# WaveMultiplierCh SuperCollider Port

Standalone SuperCollider server-plugin package for the chowdsp_utils `WaveMultiplier<float, 6>`, exposed as `WaveMultiplierCh` to match the standalone `*Ch` naming convention under `ports/`.

## Layout

- `src/WaveMultiplierCh.cpp`: server plugin implementation
- `Classes/WaveMultiplierCh.sc`: sclang class wrapper
- `HelpSource/Classes/WaveMultiplierCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

The staged extension folder is `WaveMultiplierCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the six-stage ADAA Serge-style multiplier from the SignalGenerator example. It keeps the original fixed stage count and the resulting six-sample total latency, but leaves drive and makeup gain to ordinary SC signal arithmetic around the UGen.
