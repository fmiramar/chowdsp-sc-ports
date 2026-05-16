# MouseDriveCh Porting Notes

- Source module: `BYOD/src/processors/drive/mouse_drive/MouseDrive.cpp`
- Circuit model: `BYOD/src/processors/drive/mouse_drive/MouseDriveWDF.h`
- Scope: audio-rate standalone SC UGen with `distortion` and `volume`
- Retained behavior:
  - WDF clipping circuit topology
  - distortion control mapping `1 + Rdistortion * distortion^5`
  - post-gain curve `-24 * (1 - volume) - 12` dB
  - DC blocking around 15 Hz
- Omitted:
  - BYOD UI/netlist inspection metadata
  - host-side smoothing utilities and stereo buffer wrappers
