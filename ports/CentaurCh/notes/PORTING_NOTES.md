# CentaurCh Porting Notes

- Source reference: `KlonCentaur/ChowCentaur/`
- Implemented path: `InputBuffer -> GainStageProc -> ToneFilter -> OutputStage -> DC blocker`
- First-pass omissions:
  - `GainStageMLProc` neural mode
  - plugin bypass fade
  - explicit mono fold mode
- The traditional gain stage keeps:
  - `PreAmpWDF`
  - `AmpStage`
  - `ClippingWDF` with 2x oversampling
  - `FeedForward2WDF`
  - `SummingAmp`
- One deliberate simplification: the custom diode pair uses the direct `omega4` approximation across the whole range instead of the upstream quiet-signal LUT fallback.
