# KickCh Porting Notes

- Source reference: `ChowKick/src/ChowKick.cpp` plus `dsp/{Trigger,PulseShaper,ResonantFilter,Noise,OutputFilter}.*`
- SC adaptation:
  - plugin MIDI note-on handling was replaced with explicit `trig` and `freq` inputs
  - `freq` is latched per trigger into one of four internal round-robin voices
  - plugin-only MTS and velocity-sense toggles were collapsed into a direct `velocity` input
- Fidelity notes:
  - the pulse shaper keeps the same WDF topology and resistor mappings
  - the resonant filter keeps the same coefficient formulas and three nonlinear modes
  - the output stage keeps the original tone/bounce makeup gain formula
  - noise generation is local and self-contained, including `Uniform`, `Normal`, and `Pink` modes
