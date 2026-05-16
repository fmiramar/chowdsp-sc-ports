# SimpleReverbDifCh Porting Notes

- This port keeps the original `SimpleReverb` structure more closely than `SimpleReverbCh`: convolution diffuser first, then an 8-line Householder FDN with shelving in the feedback paths and two internal LFO modulators.
- The standalone convolution stage uses a local SC-friendly partitioned-convolution core rather than the JUCE/chowdsp `ConvolutionEngine`.
- The random delay multipliers and diffuser kernels are deterministic from fixed seeds so synth construction remains reproducible inside the SC server.
- `diffusionTime` changes rebuild the partitioned impulse response, so this version is intended for static or slow-moving diffusion settings.
