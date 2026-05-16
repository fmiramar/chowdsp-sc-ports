# SimpleReverbDifCh

More faithful standalone SuperCollider port of the chowdsp `SimpleReverb` example.

## Notes

- Stereo in / stereo out.
- Uses the long convolution diffuser idea from the original example.
- Uses an 8-line Householder FDN with the original alternating stereo load/store pattern.
- Heavier than `SimpleReverbCh`, especially when `diffusionTime` changes continuously.
