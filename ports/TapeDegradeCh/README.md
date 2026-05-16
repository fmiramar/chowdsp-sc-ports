# TapeDegradeCh SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `ChowTapeDegrade` module, exposed as `TapeDegradeCh` to match the standalone `*Ch` naming convention under `ports/`.

## Layout

- `src/TapeDegradeCh.cpp`: server plugin implementation
- `src/dsp/`: local smoothing, level-detector, lowpass, noise, and RNG helpers
- `Classes/TapeDegradeCh.sc`: sclang class wrapper
- `HelpSource/Classes/TapeDegradeCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `TapeDegradeCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the standalone ChowTape degrade stage at the original module-control level:

- normalized `depth`, `amount`, `variance`, and `envelope`
- randomized degrade-noise injection
- smoothed lowpass degrade filter and smoothed gain stage
- optional envelope-followed noise
- original 64-sample parameter cook cadence

The Rack randomness is replaced with local deterministic xorshift RNGs so the stage is self-contained inside the standalone port package.
