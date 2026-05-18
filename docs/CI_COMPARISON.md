# CI comparison

This project was compared against the local SuperCollider and sc3-plugins checkouts on 2026-05-18, plus the public SuperCollider README platform-support notes.

## SuperCollider main repository

The main SuperCollider repository has separate reusable workflows for Linux, macOS, Windows, and tests. Its matrix is broader than this port repository because it builds the full language, IDE, audio servers, bundled dependencies, installers, and CTest suites.

Relevant patterns:

- Linux covers Ubuntu 22.04 and 24.04, several GCC/Clang versions, system-library variants, shared `libscsynth`, CTest jobs, and an `ubuntu-24.04-arm` job.
- macOS covers a universal `x86_64;arm64` artifact, an x64 legacy artifact, arm64 system-library testing, and shared `libscsynth`.
- Windows covers 32-bit MSVC, 64-bit MSVC, and 64-bit MinGW builds, with archive and installer paths for selected jobs.

The public SuperCollider README says current tested platforms include Windows 10 64-bit with MSVC 2022, macOS 15 with Xcode 15.2, and Ubuntu 22.04 with gcc 12. It also lists guaranteed support for Windows 10/11, MSVC 2019/2022, macOS 13-15, Xcode 14-16, Debian >= 11, Ubuntu 22.04/24.04, and gcc >= 9 / clang >= 11.

Source: https://github.com/supercollider/supercollider

## sc3-plugins repository

The sc3-plugins CI is the closest model for these ChowDSP ports because it builds SuperCollider server extensions rather than the whole SuperCollider application. Its workflow checks out SuperCollider, configures with `SC_PATH`, builds with `SUPERNOVA=ON`, installs into a staging prefix, compresses the install tree, uploads workflow artifacts, and deploys release assets on tags.

Relevant patterns:

- Linux x64 artifacts for Ubuntu 22.04 and 24.04.
- macOS artifact built as a universal `x86_64;arm64` Xcode build.
- Windows artifacts for 32-bit and 64-bit MSVC.
- Explicit CMake generator / architecture flags where the runner default is not enough.

Source: https://github.com/supercollider/sc3-plugins

## ChowDSP port CI changes

The previous ChowDSP release setup had a main workflow for Linux x64, Windows x64, and macOS arm64, plus a separate tag-only workflow for macOS x64. That made macOS x64 less visible on branch builds and left Windows/macOS architecture selection partly implicit.

The updated workflow keeps the simpler sc3-plugins-style extension packaging model but makes target availability explicit:

- `linux-x64-ubuntu-22.04`
- `linux-x64-ubuntu-24.04`
- `linux-arm64-ubuntu-24.04`
- `windows-x64`
- `windows-x86`
- `macos-x64`
- `macos-apple-silicon`

The release builder now accepts CMake generator, CMake architecture, and `CMAKE_OSX_ARCHITECTURES` arguments so CI can name the target architecture directly. The separate macOS x64 workflow was removed to avoid duplicate release uploads and keep all release targets in one matrix.
