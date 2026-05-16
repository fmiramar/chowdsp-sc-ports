# ModalCh SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `ChowModal` mode resonator, renamed to `ModalCh` to follow the `*Ch` naming convention for these ports.

## Layout

- `src/ModalCh.cpp`: server plugin implementation
- `Classes/ModalCh.sc`: sclang class wrapper
- `HelpSource/Classes/ModalCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build -DSC_PATH=../../supercollider
cmake --build build
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Plugin API mismatches are hard failures.

Example for the locally installed app here:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit. The local dev checkout here is newer than the `3.14.1` release and may use a different plugin API.

To install into a staging prefix:

```bash
cmake --install build --prefix /tmp/modalch-sc
```

The resulting extension folder is installed as `ModalCh/`.

To install manually into SuperCollider, copy the staged `ModalCh/` folder into `Platform.userExtensionDir`.
After copying, recompile the class library and reboot the server. The language and the server discover extensions at different times:

- `sclang` must see `Classes/ModalCh.sc`
- `scsynth` must load `ModalCh.scx` when the server boots

If you use `supernova`, build with `-DSUPERNOVA=ON` and install the `ModalCh_supernova.scx` target as well.

## Current Status

This first draft ports the single-mode resonator core from the VCV Rack module. It does not yet include a modal bank wrapper or automated regression tests.
