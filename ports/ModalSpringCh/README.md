# ModalSpringCh SuperCollider Port

Standalone SuperCollider server-plugin package for the `ModalSpringReverb` example plugin from `chowdsp_utils`, exposed as `ModalSpringCh` to match the standalone `*Ch` naming convention under `ports/`.

## Layout

- `src/ModalSpringCh.cpp`: server plugin implementation
- `src/dsp/mode_params.hpp`: compatibility shim for the original modal analysis table
- `Classes/ModalSpringCh.sc`: sclang class wrapper
- `HelpSource/Classes/ModalSpringCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `ModalSpringCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the chowdsp_utils modal spring example at the original effect-control level:

- bipolar pitch in octaves
- normalized decay
- dry/wet mix
- the original modulated-mode count, rate, and depth controls

The port keeps the original measured mode tables and modulation law, but uses a local scalar modal bank instead of the original xsimd implementation.
