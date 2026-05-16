# FracFilterCh

Standalone SuperCollider port of the chowdsp `FractionalOrderFilter` module.

## Notes

- Mono in / mono out.
- Exposes the raw module controls directly: `freq` in Hz and `alpha` in `0..1`.
- Uses the upstream default approximation order `N = 4`.
- Recomputes the four first-order section coefficients directly from the chowdsp pole/zero formula.
