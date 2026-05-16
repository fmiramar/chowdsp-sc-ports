# VogueLadderCh SuperCollider Port

Standalone SuperCollider server-plugin package for the BYOD ladder filter module, exposed as `VogueLadderCh` to match the port target name used in this workspace.

## Layout

- `src/VogueLadderCh.cpp`: server plugin implementation
- `src/dsp/`: local ladder filter DSP adapted from BYOD
- `Classes/VogueLadderCh.sc`: sclang class wrapper
- `HelpSource/Classes/VogueLadderCh.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Build

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH=../../supercollider-3.14.1
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Important: `SC_PATH` must point to a SuperCollider source tree that matches the runtime app you will boot.

## Current Status

This first draft ports the public BYOD ladder filter behavior:

- cascaded resonant 4-pole high-pass and low-pass ladder sections
- original normalized parameter mappings
- drive gain plus nonlinear saturation blend
- oscillating/non-oscillating resonance mode
