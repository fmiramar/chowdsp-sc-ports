# FuzzMachineCh Porting Notes

- Source module: `BYOD/src/processors/drive/fuzz_machine/FuzzMachine.cpp`
- Source assets:
  - `BYOD/res/guitar_ml_models/fuzz_15.json`
  - `BYOD/res/guitar_ml_models/fuzz_2.json`
  - `BYOD/res/guitar_ml_models/fuzz_15_88.json`
  - `BYOD/res/guitar_ml_models/fuzz_2_88.json`
- Scope:
  - audio-rate standalone SC UGen
  - conditioned 2-input LSTM runtime for the shipped public fuzz models
  - 2x oversampling for 44.1/48 kHz operation
- Retained behavior:
  - conditioned model input `[audio, fuzz]`
  - residual output mode
  - DC blocking and output level scaling
  - model 1.5 / model 2 selection
- Simplifications:
  - fixed 1x/2x sample-rate handling instead of the full BYOD arbitrary-rate resampler
  - no host-side UI metadata
