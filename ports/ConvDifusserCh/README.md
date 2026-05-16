# ConvDifusserCh

Standalone SuperCollider port of the chowdsp `ConvolutionDiffuser` stage used by the `SimpleReverb` example.

## Notes

- Stereo in / stereo out.
- Uses internally generated noise kernels rather than external IR buffers.
- `diffusionTime` changes rebuild the partitioned-convolution IR, so slow control-rate movement is the intended use.
