# GuitarMLAmpCh Porting Notes

- Source module: `BYOD/src/processors/drive/GuitarMLAmp.cpp`
- Scope reduced to the three built-in public models:
  - `BluesJrAmp_VolKnob.json`
  - `TS9_DriveKnob.json`
  - `MesaRecMini_ModernChannel_GainKnob.json`
- Omits custom model loading UI and XML persistence
- Retains conditioned LSTM inference, model selection, optional correction filter, and Mesa normalization
