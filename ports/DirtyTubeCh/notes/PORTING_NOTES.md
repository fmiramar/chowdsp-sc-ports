# DirtyTubeCh Porting Notes

- Source module: `BYOD/src/processors/drive/tube_amp/TubeAmp.cpp`
- Tube core: `BYOD/src/processors/drive/tube_amp/TubeProc.h`
- Scope: audio-rate standalone SC UGen with `drive`
- Retained behavior:
  - drive skew `drive^8`
  - input gain `5.9 * skew + 0.1`
  - output compensation `1 / (5 * skew + 1)`
  - WDF tube stage with alpha-transformed parasitic capacitors
- Simplifications:
  - mono processor only
  - direct SC sample loop instead of JUCE buffer/gain wrappers
