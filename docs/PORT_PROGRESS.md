# Port Progress

Current date: 2026-05-05

## Completed ports in this run

- `TR808OutCh`
- `VogueLadderCh`
- `Phase90Ch`
  - public BYOD fallback for unavailable add-on `Opto Phase`
- `MouseDriveCh`
- `FuzzMachineCh`
- `DirtyTubeCh`
- `BassFaceCh`
- `MetalFaceCh`
- `BlondeDriveCh`
- `JuniorBCh`
- `GuitarMLAmpCh`

## Build / staging status

These ports build and stage successfully under their local `build-3.14.1` directories:

- `ports/TR808OutCh`
- `ports/VogueLadderCh`
- `ports/Phase90Ch`
- `ports/MouseDriveCh`
- `ports/FuzzMachineCh`
- `ports/DirtyTubeCh`
- `ports/BassFaceCh`
- `ports/MetalFaceCh`
- `ports/BlondeDriveCh`
- `ports/JuniorBCh`
- `ports/GuitarMLAmpCh`

## Runtime validation status

Known environment blocker:

- `sclang` class-library compilation succeeds for the staged packages
- runtime smoke scripts consistently fail before plugin execution because builtin synthdefs fail to instantiate on the local test server:
  - `/s_new SynthDef not found`
  - this affects builtin smoke defs as well, so it is a local `sclang -> scsynth` harness issue, not evidence against the individual plugin DSP ports

Smoke scripts were used during development, but are intentionally not included in this publishable repository snapshot.

## Public BYOD availability notes

Unavailable in public checkout, so not currently portable without add-on source:

- `Opto Comp`
- `Blue-PG`
- `Pandora Drive`
- `OP Scream`
- `Stinger Fuzz`
- exact `Opto Phase`

Public fallback used:

- `Phase90Ch` instead of add-on `Opto Phase`

## Recommended next work

1. Debug the local SuperCollider synthdef-loading harness once
   - goal: get builtin smoke synthdefs to instantiate successfully
   - this unlocks real runtime validation for all completed ports

2. Public amp-model queue completed
   - `DirtyTubeCh`
   - `BlondeDriveCh`
   - `BassFaceCh`
   - `MetalFaceCh`
   - `JuniorBCh`
   - `GuitarMLAmpCh`

3. If add-on sources become available
   - port `Opto Comp`
   - port `Blue-PG`
   - port `Pandora Drive`
   - port `OP Scream`
   - port `Stinger Fuzz`
