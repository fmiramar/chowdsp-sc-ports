# ModalCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/ModalCh/`

It is intentionally not being added to `sc3-plugins`. Future ports should follow the same pattern and get their own package directory.

## Scope Chosen
First port target: a single-mode SuperCollider UGen that matches the VCV `ChowModal` DSP core, not a full modal bank.

Proposed public API:

```supercollider
ModalCh.ar(in, freq = 440, decay = 1.4, amp = 0.25, phase = 0, mul = 1, add = 0)
```

Notes:
- `freq` is in Hz, not the VCV semitone-knob mapping.
- `decay` is a T60 time in seconds.
- `amp` and `phase` stay explicit because they are part of the complex excitation coefficient.

## Closest Existing SuperCollider UGens
Closest DSP comparison:
- `Ringz` in `supercollider/server/plugins/FilterUGens.cpp`

Closest bank/composite model:
- `Klank` / `DynKlank` in `supercollider/server/plugins/OscUGens.cpp` and `supercollider/SCClassLibrary/Common/Audio/FSinOsc.sc`

Closest implementation patterns:
- `Streson` in `sc3-plugins/source/BhobUGens/BhobFilt.cpp` for audio/control-rate parameter handling
- `Membrane` in `sc3-plugins/source/MembraneUGens/Membrane.cpp` for standalone plugin layout

Why `Ringz` is only a reference:
- ChowModal uses the recursion `y[n] = A * x[n] + c * y[n - 1]`
- output is `imag(y[n])`
- that is not the same transfer function as `Ringz`, and `Ringz` has no explicit complex amplitude input

## DSP Decisions
1. Port the `Mode.hpp` recursion directly instead of approximating with `Ringz`.
2. Use explicit real/imag state in the sample loop instead of `std::complex`.
3. Support audio-rate modulation for `freq`, `decay`, `amp`, and `phase`.
4. Do not include the Rack-side DC blocker in the port. Document that SC users should add `LeakDC` or `HPF` themselves when needed.
5. Keep `zapgremlins` on the internal state after each block.

## Current Limits
- no modal bank wrapper yet
- no regression test harness yet
- no sonic A/B verification against the Rack version yet
- build compatibility depends on matching the plugin API of the runtime SuperCollider app

## Compatibility Note
The installed SuperCollider app on this machine is `3.14.1` (`426edf6`) and expects plugin API `3`.

The local `supercollider/` checkout is newer (`Version-3.14.1-201-g21347a84c`) and exports plugin API `5`, so binaries built against it will not load in the installed app.

For local testing, use:

- `SC_PATH=../../supercollider-3.14.1`
- staged output: `ports/ModalCh/build-3.14.1/stage/ModalCh/`

## Next Recommended Step
Build and listen-test this standalone package, then add:

1. an impulse-response regression test
2. a sine-response test for on-frequency vs off-frequency excitation
3. a higher-level modal-bank wrapper once the single-mode core is verified
