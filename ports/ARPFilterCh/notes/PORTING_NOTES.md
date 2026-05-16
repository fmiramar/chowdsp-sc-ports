# ARPFilterCh Porting Notes

- The DSP follows the chowdsp `ARPFilter` wrapper closely: Cytomic-style state-variable core, optional `limit mode`, and the special notch offset path.
- The example plugin's `notchOffset` parameter is a unipolar `PercentParameter`, so this port keeps `notchOffset` in `0..1` rather than exposing the underlying algorithm's wider bipolar range.
- Control-rate `notchOffset` is smoothed over `20 ms` to match the example plugin's `SmoothedBufferValue` handling.
- The port is mono per instance. SuperCollider multichannel expansion remains the normal way to use it on stereo material.
