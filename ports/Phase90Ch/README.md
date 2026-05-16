# Phase90Ch SuperCollider Port

Standalone SuperCollider server-plugin package for the public BYOD `Phaser4` module, exposed here as `Phase90Ch`.

## Layout

- `src/Phase90Ch.cpp`: server plugin implementation
- `src/dsp/`: local filter and modulation helpers adapted from BYOD phaser code
- `Classes/Phase90Ch.sc`: sclang class wrapper
- `HelpSource/Classes/Phase90Ch.schelp`: user documentation
- `notes/PORTING_NOTES.md`: architecture notes and follow-up work

## Current Status

This first draft ports the core public BYOD phaser behavior:

- 4-stage allpass phaser structure
- internal triangle LFO with source-style shaping
- selectable feedback stage
- wet/dry mix
- optional stereo dual-mono output
