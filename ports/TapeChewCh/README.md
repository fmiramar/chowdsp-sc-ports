# TapeChewCh SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `ChowTapeChew` module, exposed as `TapeChewCh` to match the standalone `*Ch` naming convention under `ports/`.

## Layout

- `src/TapeChewCh.cpp`: server plugin implementation
- `src/dsp/`: local smoothing, random, lowpass, and dropout helpers
- `Classes/TapeChewCh.sc`: sclang class wrapper
- `HelpSource/Classes/TapeChewCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `TapeChewCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the standalone ChowTape chew stage at the original module-control level:

- normalized `depth`, `frequency`, and `variance`
- dropout shaping and smoothed lowpass filtering
- randomized wet/dry chew timing
- original 64-sample parameter cook cadence

The Rack randomness is replaced with local deterministic xorshift RNGs so the stage is self-contained inside the standalone port package. The randomized wet/dry timing also includes a guard against zero-width duration ranges.
