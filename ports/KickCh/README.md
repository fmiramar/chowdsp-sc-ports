# KickCh

`KickCh` is a standalone SuperCollider port of the ChowDSP `ChowKick` plugin.

This first pass keeps the original mono kick voice architecture:

- trigger pulse generator
- WDF pulse shaper
- nonlinear resonant filter with `Linear`, `Basic`, and `Bouncy` modes
- filtered noise burst
- output tone filter and DC blocker

The main SC-specific adaptation is trigger handling: instead of plugin MIDI note-ons, `KickCh` takes `trig` and `freq` inputs and latches `freq` on each rising trigger. It still maintains up to four internal voices so overlapping kick tails behave much closer to the original plugin than a single-voice translation would.
