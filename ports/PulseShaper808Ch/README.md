# PulseShaper808Ch SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `ChowPulse` module, renamed to `PulseShaper808Ch` to follow the `*Ch` naming convention for these ports.

## Layout

- `src/PulseShaper808Ch.cpp`: server plugin implementation
- `src/dsp/`: local WDF support copied or adapted from the VCV module
- `Classes/PulseShaper808Ch.sc`: sclang class wrapper
- `HelpSource/Classes/PulseShaper808Ch.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `PulseShaper808Ch/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the active ChowPulse behavior:

- trigger-to-pulse conversion
- TR-808 pulse shaping WDF circuit
- width and decay mappings from the VCV module
- double-tap shaping on the negative swing

The trigger detector is implemented here as a Schmitt-style rising-edge detector with a `1.0 V` high threshold and `0.1 V` low threshold, matching the usual Rack convention as closely as possible without depending on Rack headers.
