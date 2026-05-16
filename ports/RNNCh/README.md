# RNNCh SuperCollider Port

Standalone SuperCollider server-plugin package for the ChowDSP `ChowRNN` module, exposed as `RNNCh` to match the standalone `*Ch` naming convention under `ports/`.

## Layout

- `src/RNNCh.cpp`: server plugin implementation
- `src/dsp/`: local fixed-size RNN helpers and the output DC blocker
- `Classes/RNNCh.sc`: sclang class wrapper
- `HelpSource/Classes/RNNCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot. Do not build this extension against the moving `supercollider/` development checkout unless your installed app was built from the same commit.

The staged extension folder is `RNNCh/`. Copy that folder into `Platform.userExtensionDir`, recompile the class library, and reboot the server.

## Current Status

This first draft ports the ChowRNN module at the inference level:

- four signal inputs feeding the original `Dense -> Tanh -> GRU -> Dense` topology
- deterministic seed-based weight randomization on construction and explicit `randomize` triggers
- explicit `reset` trigger for the recurrent state
- Rack-style output DC blocker with zero output-layer bias
- an SC approximation of the Rack jack-count makeup gain

Unlike the Rack module, this standalone port does not expose JSON save/load of the model weights. It uses a local fixed-size implementation instead of the generic MLUtils container graph, and it intentionally avoids the Rack-side UI button semantics by making randomization and reset explicit signal inputs.
