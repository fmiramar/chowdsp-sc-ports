# TR808OutCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/TR808OutCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
Port target: the standalone TR-808 output filter block, exposed as:

```supercollider
TR808OutCh.ar(in, volume = 0.8, tone = 0.5, mul = 1, add = 0)
```

This scope includes:

1. the original WDF circuit structure
2. the source plugin's `volume` taper mapping
3. the source plugin's `tone` taper mapping
4. audio-rate modulation for both controls

## Source Mapping
Relevant source files:

- `WaveDigitalFilters/TR_808/OutputFilter/src/TR808OutputFilter.h`
- `WaveDigitalFilters/TR_808/OutputFilter/src/TR808OutputFilter.cpp`

## Architecture Decisions
1. Keep the port self-contained under `ports/TR808OutCh/`.
2. Port the WDF output filter directly.
3. Preserve the normalized `0..1` control ranges from the source plugin for the first pass.
4. Vendor the richer local WDF support into this package because `ports/shared/wdf/wdf_t.h` does not currently include the `YParameterT` and current-source nodes used by this circuit.
5. For embedded C++ DSP members in SC units, use placement new with default-initialization (`new (unit) TR808OutCh;`), not value-initialization (`new (unit) TR808OutCh();`), so the server-owned `Unit` base is not clobbered.

## Compatibility Note
The installed SuperCollider app on this machine is `3.14.1` (`426edf6`) and expects plugin API `3`.

For local testing, use:

- `SC_PATH=../../supercollider-3.14.1`
- staged output: `ports/TR808OutCh/build-3.14.1/stage/TR808OutCh/`

## Next Recommended Step
After compile and listening validation:

1. compare the frequency response against the original plugin for a few `volume` and `tone` settings
2. decide whether to expose a convenience `.kr` wrapper later, or keep the interface audio-rate only
