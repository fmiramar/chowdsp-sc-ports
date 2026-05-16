# BlondeDriveCh Porting Notes

- Source module: `BYOD/src/processors/drive/BlondeDrive.cpp`
- Port uses scalar Newton iteration instead of the source SIMD implementation
- Includes character bell filter, drive inversion, bias offset, and optional higher-iteration mode
