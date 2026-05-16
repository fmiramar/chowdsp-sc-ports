# Phase90Ch Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/Phase90Ch/`

## Scope Chosen
Public fallback target while the user checks the add-on-only `Opto Phase` source.

The source used here is the public BYOD `Phaser4` module, re-exposed as:

```supercollider
Phase90Ch.ar(in, rate = 1, depth = 1, feedback = 0.6, mix = 0.5, fbStage = 2, stereo = 0, mul = 1, add = 0)
```

## Architecture Decisions
1. Port the public 4-stage BYOD phaser instead of blocking on the unavailable `Opto Phase`.
2. Keep the first pass self-contained and JUCE-free.
3. Preserve the useful signal path controls and drop BYOD's extra modulation output for now.
4. Use a simple internal LFO implementation with the same shaping curve, rather than porting the full BYOD modulation framework.
