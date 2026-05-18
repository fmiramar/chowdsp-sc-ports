# ChowDSP SuperCollider Ports

This repository contains standalone SuperCollider ports of DSP modules derived from the public work of Jatin Chowdhury / ChowDSP:

- https://github.com/Chowdhury-DSP
- https://chowdsp.com/

These plugins and modules are well known in the audio plugin world for high-quality open-source DSP, circuit modeling, tape/process emulation, drum synthesis, nonlinear filters, and neural-model effects.

Thanks a lot to Jatin Chowdhury for such an amazing effort and result, and especially for making this work public and open source.

This repository repackages that work as SuperCollider server extensions so the processors can be used directly as UGens.

This repository structure, packaging, and documentation pass were prepared with Codex / ChatGPT using a GPT-5-class coding model.

## List of UGens

Below is the current buildable UGen set included in this repository.

- `ARPFilterCh`: An ARP-inspired analog filter model for subtractive synthesis and tone shaping. It focuses on nonlinear filter color rather than a neutral textbook response.
- `AutoWahCh`: An envelope-driven wah effect derived from ChowDSP/BYOD-style pedal processing. It tracks input level and moves a resonant filter so picking dynamics become timbral motion.
- `BassFaceCh`: A heavy bass distortion neural model from BYOD. It uses a recurrent model so the output depends on recent signal history as well as the current sample and gain control.
- `BaxandallCh`: A Baxandall tone-control processor for broad bass and treble shaping. It behaves more like a musical amplifier EQ section than a narrow parametric equalizer.
- `BlondeDriveCh`: A voiced drive stage based on BYOD Blonde Drive. It combines filtering and nonlinear gain so the distortion color changes with the source and control settings.
- `CentaurCh`: A Klon Centaur-style overdrive and preamp model. It keeps the staged filtering, diode clipping, tone control, and output conditioning that make the circuit more than a simple clipper.
- `ChorusCh`: A stereo BBD-style chorus based on the ChowDSP chorus module. It uses modulated short delays, feedback, and bandwidth-limited delay coloration for vintage movement and width.
- `ConvDifusserCh`: A convolution diffuser for early-smear and spatial softening effects. It turns sharp onsets into denser, less point-like reflections before a larger reverb or delay chain.
- `DelayBBCh`: A mono bucket-brigade-style delay. It models the darker and less ideal behavior of BBD delay lines instead of acting like a transparent digital delay.
- `DiodeClipperCh`: A diode clipping stage for analog-drive sounds. It uses wave-digital-style nonlinear solving to keep the diode behavior tied to a circuit model.
- `DirtyTubeCh`: A tube-drive processor based on BYOD Dirty Tube. It models a dynamic tube stage with memory and filtering rather than applying a static waveshaper.
- `DistortCh`: A nonlinear distortion processor based on the ChowDSP ChowDer path. It combines EQ, bias, drive, clipping, and oversampling so the controls interact inside one voiced distortion structure.
- `FDNCh`: A feedback-delay-network reverb core. It builds density through delay-line feedback and mixing rather than through a single comb or allpass chain.
- `FracFilterCh`: A fractional and nonlinear filter processor. It is useful for filter colors that move beyond the usual one-pole, state-variable, or biquad families.
- `FuzzMachineCh`: A neural fuzz model from BYOD Fuzz Machine. It uses a conditioned recurrent network, so the fuzz responds to control values and recent audio context.
- `GuitarMLAmpCh`: A set of public GuitarML neural amplifier models. It exposes learned amp and drive responses as SuperCollider UGens with the model normalization needed by the upstream assets.
- `HardClipCh`: A hard clipping waveshaper. It gives abrupt limiting and bright harmonics, useful when a sharper edge is desired than soft saturation.
- `JuniorBCh`: A Pro Junior-inspired preamp stage model from BYOD Junior B. It combines circuit structure and nonlinear stages to capture amp-preamp compression and saturation.
- `KickCh`: A ChowKick-derived kick drum synthesizer. It combines trigger shaping, resonant body filtering, noise attack, tone filtering, and output conditioning in one drum voice.
- `LCOscCh`: A low-complexity oscillator and tone generator. It is meant for efficient synthetic tones where the ChowDSP oscillator shape is the useful character.
- `MetalFaceCh`: A heavy distortion neural model from BYOD Metal Face. It targets aggressive guitar/bass coloration with recurrent-state behavior rather than a fixed transfer curve.
- `ModalCh`: A modal resonator for body and struck-object tones. It creates sound from resonant modes, closer to a bank of ringing bodies than to a conventional filter.
- `ModalSpringCh`: A modal spring-style resonator. It uses modal behavior to approximate springy, metallic resonance without requiring a full physical spring simulation.
- `MouseDriveCh`: A RAT-style drive model from BYOD Mouse Drive. It uses wave-digital circuit modeling so the filter, gain, and diode branch interact.
- `PassiveLPFCh`: A passive low-pass filter model. It emphasizes simple analog-style tone loss and circuit coloration rather than steep digital filtering.
- `Phase90Ch`: A Phase 90-style phaser based on BYOD Phaser4. It uses moving allpass stages and feedback to create the familiar notched sweep.
- `PhaserFBCh`: A feedback phaser section from the ChowDSP phaser family. It focuses on the feedback path that sharpens moving notches and increases phase coloration.
- `PhaserModCh`: A modulated phaser section from the ChowDSP phaser family. It exposes LDR-shaped allpass modulation for patching custom phaser structures.
- `PolygonalOscCh`: A polygonal oscillator. It generates geometric oscillator shapes that provide a different harmonic structure from standard saw, square, and sine waves.
- `PulseShaper808Ch`: A TR-808-style pulse shaper stage. It turns triggers into analog-inspired excitation pulses suitable for drum resonators and percussive patches.
- `RNNCh`: A compact recurrent neural network sound processor. It is a small stateful nonlinear model with four inputs, useful for experimental learned dynamics and tone shaping.
- `SallenKeyCh`: A Sallen-Key filter model. It follows an analog filter topology rather than a generic biquad interface.
- `SimpleReverbCh`: A compact reverb processor. It provides an efficient ChowDSP-style room/tail effect for everyday spatial processing.
- `SimpleReverbDifCh`: A diffuser-enhanced variant of the simple reverb. It adds early diffusion so attacks smear before entering the reverb tail.
- `TR808HatCh`: A TR-808-inspired hi-hat voice component. It models the resonant/noisy analog character needed for metallic hat tones.
- `TR808OutCh`: A TR-808 output filter and output-stage processor. It is intended as the final tone-conditioning stage for 808-style drum chains.
- `TR808SnareCh`: A TR-808-inspired snare resonator component. It supplies the pitched body stage that can be excited by pulses, noise, or layered drum sources.
- `TanhClipCh`: An ADAA tanh soft clipper. It keeps the smooth saturation curve of `tanh` while reducing aliasing compared with naive sample-by-sample waveshaping.
- `TapeCh`: A tape-style coloration processor. It focuses on the nonlinear and filtering character of tape processing rather than full transport simulation.
- `TapeChewCh`: A tape chew and warble degradation effect. It adds unstable pitch and time movement associated with damaged or worn tape playback.
- `TapeCompCh`: A tape compression processor. It uses level detection and gain movement to capture dynamics behavior rather than only saturation.
- `TapeDegradeCh`: A tape degradation processor. It provides worn-medium effects such as unstable tone, texture, and bandwidth loss.
- `TapeLossCh`: A tape loss and dulling processor. It focuses on the bandwidth and high-frequency loss side of tape degradation.
- `VogueLadderCh`: A ladder filter model from BYOD Vogue Ladder. It emphasizes nonlinear resonant filter color rather than a transparent digital ladder approximation.
- `WarpCh`: A nonlinear warped filter and distortion processor. It uses an oversampled recursive nonlinear structure where cutoff, heat, width, and drive interact.
- `WaveMultiplierCh`: A wave multiplier for West Coast-style nonlinear synthesis. It increases harmonic complexity by folding and multiplying waveform shape rather than clipping peaks flat.
- `WernerCh`: A generalized nonlinear state-variable filter based on ChowDSP Werner. It combines resonance, damping, and drive inside one recursive filter state.
- `WestCoastFoldCh`: A West Coast wavefolder. It bends waveform peaks back into the signal to create animated harmonic growth from simple oscillators.

## DSP Strategies

These ports mostly translate ChowDSP algorithms into standalone SuperCollider server plugins. The main strategies are antiderivative antialiasing for waveshapers, explicit oversampling around sharp nonlinear stages, wave-digital circuit solving for diode and tube networks, BBD-style delay/filter modeling, feedback-delay networks for reverb density, modal resonators for physical body tones, and recurrent neural networks for learned amp and fuzz responses.

Compared with many traditional SuperCollider UGens, these ports are often more stateful, circuit-specific, and control-mapping-specific. They try to preserve upstream parameter laws, sample-rate assumptions, feedback behavior, and model normalization instead of exposing only generic DSP building blocks.

The similarity is that they still behave like normal server-side UGens: they run inside `scsynth`, accept audio/control-rate inputs, install as ordinary extensions, and can be patched with vanilla UGens such as `SinOsc`, `Saw`, `Ringz`, `DelayN`, `RLPF`, `LeakDC`, and `FreeVerb`.

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
  --platform macos-apple-silicon
```

## CI / Releases

GitHub Actions is configured to build release artifacts for:

- `ubuntu-22.04` (`linux-x64-ubuntu-22.04`)
- `ubuntu-24.04` (`linux-x64-ubuntu-24.04`)
- `ubuntu-24.04-arm` (`linux-arm64-ubuntu-24.04`)
- `windows-2022` (`windows-x64`)
- `windows-2022` (`windows-x86`)
- `macos-13` (`macos-x64`)
- `macos-14` (`macos-apple-silicon`)

The workflow:

1. clones SuperCollider `Version-3.14.1`
2. builds every port under `ports/` that has a `CMakeLists.txt`
3. installs them into a staged `Extensions/` tree
4. zips that tree into a platform-specific release asset
5. uploads the asset to the GitHub Release for tagged builds

## Notes

- This publishable repo intentionally does not include the local smoke-test bundle used during development.
- Some public BYOD add-on modules are not included because their source was not available in the public checkout.

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
