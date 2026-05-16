# SimpleReverbCh SuperCollider Port

Standalone SuperCollider server-plugin package for the `SimpleReverb` example plugin from `chowdsp_utils`, exposed as `SimpleReverbCh` to match the standalone `*Ch` naming convention under `ports/`.

## Layout

- `src/SimpleReverbCh.cpp`: server plugin implementation
- `Classes/SimpleReverbCh.sc`: sclang class wrapper
- `HelpSource/Classes/SimpleReverbCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

The staged extension folder is `SimpleReverbCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the chowdsp_utils `SimpleReverb` example at the original effect-control level:

- diffusion time in milliseconds
- base FDN delay time in milliseconds
- low and high T60 times in milliseconds
- normalized modulation amount
- dry/wet mix

The port keeps a deterministic stereo 8-line FDN with the example's internal LFO modulation idea, but uses a lighter stereo diffuser chain instead of the original long convolution diffuser.
