# ARPFilterCh

Standalone SuperCollider port of the chowdsp `ARPFilter` example.

## Notes

- Mono in / mono out.
- Keeps the original multimode SVF core plus the ARP-specific `limit mode`.
- `mode` matches the example plugin: `0 = LPF2`, `1 = BPF2`, `2 = HPF2`, `3 = Notch`.
- Control-rate `notchOffset` is smoothed over `20 ms` like the example plugin parameter.
