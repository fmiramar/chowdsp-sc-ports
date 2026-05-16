# RNNCh Porting Notes

## Package Decision
This port lives in its own standalone package:

- `ports/RNNCh/`

It is intentionally not being added to `sc3-plugins`. The current naming convention for these ports is `*Ch`.

## Scope Chosen
First port target: the live four-input ChowRNN inference path, exposed as:

```supercollider
RNNCh.ar(in1, in2 = 0, in3 = 0, in4 = 0, randomize = 0, seed = 0, reset = 0, mul = 1, add = 0)
```

This scope includes:

1. the original network topology: `Dense(4) -> Tanh(4) -> GRU(4) -> Dense(1)`
2. deterministic weight randomization using a seed value
3. recurrent-state reset
4. the output DC blocker and zero output-layer bias

This scope does not yet include:

1. Rack JSON save/load of weight matrices
2. a language-side API for exporting/importing exact weight tensors
3. numeric comparison against the original MLUtils implementation

## Source Mapping
Relevant source files:

- `ChowDSP-VCV/src/ChowRNN/ChowRNN.cpp`
- `ChowDSP-VCV/src/ChowRNN/LayerRandomiser.cpp`
- `ChowDSP-VCV/src/ChowRNN/LayerRandomiser.hpp`
- `ChowDSP-VCV/lib/MLUtils/Model.h`
- `ChowDSP-VCV/lib/MLUtils/dense.h`
- `ChowDSP-VCV/lib/MLUtils/activation.h`
- `ChowDSP-VCV/lib/MLUtils/gru.h`
- `ChowDSP-VCV/lib/MLUtils/gru.cpp`
- `ChowDSP-VCV/doc/manual.md`

## Architecture Decisions
1. Keep the port self-contained under `ports/RNNCh/`.
2. Reimplement the fixed-size `4 -> 4 -> 4 -> 1` network locally rather than embedding the generic MLUtils layer graph.
3. Replace the Rack randomize button with a rising-edge `randomize` trigger and deterministic `seed` input so the server plugin is reproducible from code.
4. Keep a separate `reset` trigger because recurrent-state reset is useful even when the weights stay the same.
5. Approximate the Rack module's connected-jack makeup gain from which SC inputs are not scalar zero, because SC server plugins do not expose jack connection state.

## Known Difference
The original Rack-side `LayerRandomiser` helpers build temporary pointer arrays incorrectly when calling the MLUtils `setWeights`/`setWVals` APIs. The standalone port intentionally does not preserve that bug; it writes randomized weights directly into a local fixed-size model.
