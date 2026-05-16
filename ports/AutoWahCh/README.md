# AutoWahCh SuperCollider Port

Standalone SuperCollider server-plugin package for the `AutoWah` example plugin from `chowdsp_utils`, exposed as `AutoWahCh` to match the standalone `*Ch` naming convention under `ports/`.

## Layout

- `src/AutoWahCh.cpp`: server plugin implementation
- `src/dsp/`: local level-detector and modulated peaking-filter helpers
- `Classes/AutoWahCh.sc`: sclang class wrapper
- `HelpSource/Classes/AutoWahCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `AutoWahCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the chowdsp_utils auto-wah example at the original effect-control level:

- base frequency in Hz
- Q and gain in dB
- attack and release in milliseconds
- normalized sweep amount

The port uses a local scalar version of the chowdsp modulated peaking-filter wrapper and keeps the original envelope-followed sweep law.
