# ChowDSP SuperCollider Ports

This repository contains standalone SuperCollider ports of DSP modules derived from the public work of Jatin Chowdhury / ChowDSP:

- https://github.com/Chowdhury-DSP
- https://chowdsp.com/

These plugins and modules are well known in the audio plugin world for high-quality open-source DSP, circuit modeling, tape/process emulation, drum synthesis, nonlinear filters, and neural-model effects.

Thanks a lot to Jatin Chowdhury for such an amazing effort and result, and especially for making this work public and open source.

This repository repackages that work as SuperCollider server extensions so the processors can be used directly as UGens.

This repository structure, packaging, and documentation pass were prepared with Codex / ChatGPT using a GPT-5-class coding model.

No personal data, machine-local paths, private tokens, or user-specific environment details are intentionally included in this publishable snapshot.

## License

This repository is set up for `GPL-3.0-only`.

Reason:

- several ports here are derived from the public `BYOD` codebase, which is licensed under GPL v3
- other upstream ChowDSP projects included here use BSD-3-Clause, which is GPL-compatible
- using GPL v3 for the whole repository is the conservative and consistent distribution choice

See:

- [LICENSE](LICENSE)
- [NOTICE.md](NOTICE.md)
- [LICENSES/upstream](LICENSES/upstream)

## Included UGens

Below is the current UGen set included in this repository.

- `ARPFilterCh`: analog-style ARP-inspired filter
- `AutoWahCh`: envelope-driven wah / auto-filter effect
- `BassFaceCh`: heavy bass distortion neural model from BYOD
- `BaxandallCh`: Baxandall EQ / tone stack processor
- `BlondeDriveCh`: voiced drive stage based on BYOD Blonde Drive
- `CentaurCh`: Klon Centaur-style overdrive / preamp
- `ChorusCh`: chorus modulation effect
- `ChowFDNCh`: feedback-delay-network reverb / spatial effect
- `ConvDifusserCh`: convolution diffuser / smear processor
- `DelayBBCh`: bucket-brigade / analog-style delay
- `DiodeClipperCh`: diode clipping distortion stage
- `DirtyTubeCh`: tube amplifier drive stage from BYOD Dirty Tube
- `DistortCh`: nonlinear distortion processor
- `FDNCh`: feedback-delay-network effect core
- `FracFilterCh`: fractional / nonlinear filter processor
- `FuzzMachineCh`: neural fuzz model from BYOD Fuzz Machine
- `GuitarMLAmpCh`: built-in public GuitarML neural amp models
- `HardClipCh`: hard clipping waveshaper
- `JuniorBCh`: Pro Junior-inspired preamp stage model from BYOD Junior B
- `KickCh`: kick drum synthesizer / resonant drum voice
- `LCOscCh`: low-complexity oscillator / tone generator
- `MetalFaceCh`: heavy distortion neural model from BYOD Metal Face
- `ModalCh`: modal resonator / resonant body processor
- `ModalSpringCh`: modal spring / spring-like resonator
- `MouseDriveCh`: RAT-style clipping drive from BYOD Mouse Drive
- `PassiveLPFCh`: passive low-pass filter model
- `Phase90Ch`: Phase 90-style phaser based on public BYOD Phaser4
- `PhaserFBCh`: feedback phaser
- `PhaserModCh`: modulated phaser
- `PolygonalOscCh`: polygonal / geometric oscillator
- `PulseShaper808Ch`: TR-808 pulse shaper stage
- `RNNCh`: small recurrent neural network sound processor
- `SallenKeyCh`: Sallen-Key filter model
- `SimpleReverbCh`: compact reverb
- `SimpleReverbDifCh`: simple reverb diffuser variant
- `TR808HatCh`: TR-808 hi-hat voice
- `TR808OutCh`: TR-808 output filter / output stage
- `TR808SnareCh`: TR-808 snare voice
- `TanhClipCh`: tanh soft clipper
- `TapeCh`: tape-style processor / coloration
- `TapeChewCh`: tape chew / warble degradation effect
- `TapeCompCh`: tape compression / saturation processor
- `TapeDegradeCh`: tape degradation effect
- `TapeLossCh`: tape loss / dulling / bandwidth loss processor
- `VogueLadderCh`: ladder filter from BYOD Vogue Ladder
- `WarpCh`: nonlinear warp / distortion processor
- `WaveMultiplierCh`: wave multiplier / West Coast-style nonlinear processor
- `WernerCh`: resonant / nonlinear filter processor
- `WestCoastFoldCh`: West Coast wavefolder

## Installing Release Builds

Each release archive is packaged with this layout:

```text
ChowDSP-SC-Ports-<version>-<platform>/
  Extensions/
    <PortName>/
      <PortName>.scx
      Classes/
      HelpSource/
      models/   # when needed
```

Install by copying the contents of `Extensions/` into your SuperCollider user extension directory.

In SuperCollider, evaluate:

```supercollider
Platform.userExtensionDir
```

Then recompile the class library.

## Building Locally

The CI workflow clones SuperCollider source and builds each port independently. You can do the same locally:

```bash
python3 scripts/build_release.py \
  --sc-path /path/to/supercollider \
  --build-root build \
  --stage-root stage/Extensions
```

Then create a release archive:

```bash
python3 scripts/package_release.py \
  --stage-root stage/Extensions \
  --output-dir dist \
  --version dev \
  --platform macos-arm64
```

## CI / Releases

GitHub Actions is configured to build release artifacts for:

- `ubuntu-24.04` (`linux-x64`)
- `windows-2022` (`windows-x64`)
- `macos-13` (`macos-x64`)
- `macos-14` (`macos-arm64`)

The workflow:

1. clones SuperCollider `Version-3.14.1`
2. builds every port under `ports/` that has a `CMakeLists.txt`
3. installs them into a staged `Extensions/` tree
4. zips that tree into a platform-specific release asset
5. uploads the asset to the GitHub Release for tagged builds

## Notes

- This publishable repo intentionally does not include the local smoke-test bundle used during development.
- Some public BYOD add-on modules are not included because their source was not available in the public checkout.
