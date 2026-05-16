# PolygonalOscCh

Standalone SuperCollider port of the chowdsp `PolygonalOscillator` example.

## Notes

- Source UGen with the original `freq`, `order`, `teeth`, and `gainDB` control surface.
- Keeps the original non-band-limited polygonal waveform behavior, so aliasing at higher frequencies is expected.
- `gainDB` is smoothed over `50 ms` like the example plugin's gain stage.
- The oscillator divisor is guarded against exact zero to avoid `inf`/`nan` samples in the server.
