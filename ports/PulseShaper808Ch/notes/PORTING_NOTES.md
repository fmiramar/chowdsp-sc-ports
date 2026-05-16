# PulseShaper808Ch Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/PulseShaper808Ch/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the active ChowPulse behavior from the VCV module, exposed as:

```supercollider
PulseShaper808Ch.ar(trig, width = 0.5, decay = 0.5, doubleTap = 0, mul = 1, add = 0)
```

This scope includes:

1. trigger-to-pulse conversion
2. width and decay mappings from the VCV module
3. the WDF pulse-shaper circuit
4. the double-tap post-shaping applied to the negative swing

## Source Mapping
Relevant source files:

- `ChowDSP-VCV/src/ChowPulse/ChowPulse.cpp`
- `ChowDSP-VCV/src/ChowPulse/PulseShaper.hpp`

## Architecture Decisions
1. Keep the port self-contained under `ports/PulseShaper808Ch/`.
2. Port the WDF pulse shaper directly.
3. Preserve the normalized VCV control ranges for the first pass.
4. Implement the trigger stage as a Schmitt-style rising-edge detector with `1.0 V` high and `0.1 V` low thresholds, because the Rack helper itself is not locally available in this workspace.
5. Keep the SC interface audio-rate only for now.
6. For embedded C++ DSP members in SC units, use placement new with default-initialization (`new (unit) PulseShaper808Ch;`), not value-initialization (`new (unit) PulseShaper808Ch();`), so the server-owned `Unit` base is not clobbered.

## Compatibility Note
The installed SuperCollider app on this machine is `3.14.1` (`426edf6`) and expects plugin API `3`.

For local testing, use:

- `SC_PATH=../../supercollider-3.14.1`
- staged output: `ports/PulseShaper808Ch/build-3.14.1/stage/PulseShaper808Ch/`

## Next Recommended Step
After compile and listening validation:

1. confirm the trigger detector behavior against the original Rack module
2. compare the envelope shape for a few width/decay settings
3. decide whether the class should later expose physical-unit arguments in addition to the normalized controls
