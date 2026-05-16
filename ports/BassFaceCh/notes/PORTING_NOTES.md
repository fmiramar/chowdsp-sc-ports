# BassFaceCh Porting Notes

- Source module: `BYOD/src/processors/drive/BassFace.cpp`
- Model assets:
  - `bass_face_model_96k.json`
  - `bass_face_model_88_2k.json`
- Uses conditioned residual LSTM inference with optional 2x oversampling at `<= 48 kHz`
