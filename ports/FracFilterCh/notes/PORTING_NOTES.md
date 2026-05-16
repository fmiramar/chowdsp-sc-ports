# FracFilterCh Porting Notes

- There is no dedicated chowdsp example plugin for `FractionalOrderFilter`, so this port uses the module itself as the source of truth.
- The port keeps the upstream default approximation order `N = 4` rather than exposing the template order as a runtime control.
- The DSP is implemented locally from the same pole/zero derivation and first-order bilinear transform used by the chowdsp module, instead of pulling in the larger filter utility stack.
- `freq` and `alpha` both accept control-rate or audio-rate inputs. When either is audio-rate, coefficients are recomputed sample by sample.
