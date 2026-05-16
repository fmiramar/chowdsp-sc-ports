# PolygonalOscCh Porting Notes

- The DSP follows the chowdsp `PolygonalOscillator` example closely: the same continuous-order polygon formula, the same `freq/order/teeth` parameter set, and the same `gainDB` control that the example plugin applies after generation.
- The original chowdsp implementation is explicitly still a WIP and not alias-suppressed; this port keeps that behavior rather than trying to redesign it into a band-limited oscillator.
- The standalone port adds one safety guard that is worth documenting: the divisor in the oscillator formula is clamped away from exact zero so the server cannot produce `inf` or `nan` samples on singular points.
