# ConvDifusserCh Porting Notes

- The kernel-generation behavior follows the chowdsp `ConvolutionDiffuser` design: random-noise IR, short attack ramp, truncation by diffusion time, and `32 / sqrt(N)` makeup gain.
- The standalone port uses a local SC-friendly partitioned convolution core instead of the JUCE/chowdsp `ConvolutionEngine`.
- Random kernel generation is deterministic from fixed seeds so synth instantiation stays reproducible and avoids `std::random_device` in the server path.
