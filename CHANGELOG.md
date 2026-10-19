# Changelog

All notable changes to this project are documented in this file.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- Genre profiles (11 macro-genres) driving presets, suggestions, rhythm, bass and melody style.
- Macro-controlled melody generator (Energy, Motion, Tension, Contour, Register).
- Ensemble arranging (string quartet/orchestra, symphony orchestra, SATB choir, brass) with four-part voice
  leading and polyharmony (extensions and polychords); multitrack *Arrangement .mid* export.
- AGPL-3.0 license, contributing guide, code of conduct, security policy, issue and pull request templates.

### Changed
- Documentation translated to English; Windows guide moved to `docs/BUILDING_WINDOWS.md`.
- README restructured after the Steinberg VST 3 SDK layout; VST trademark attribution and VST Compatible Logo
  per the Steinberg VST Usage Guidelines; third-party notices added.
- CI: GitHub Actions pinned to commit SHAs; engine tests also run under ASan/UBSan.

### Security
- Plug-in state loaded from a project is now size-limited and range-checked: crafted note positions could
  cause signed integer overflow, and unbounded note lists could exhaust memory.
- Guard against a zero sample rate reported by the host.

## [0.1.0]

### Added
- First version of the ChordMate VST3/AU plugin for Ableton Live 12: scales, next-chord suggestions,
  progressions, voice leading, melody generation, MIDI capture with quantisation and harmonisation,
  live chord/key detection and drag-and-drop MIDI export.

[Unreleased]: https://github.com/ravaninip/ChordMate/commits/main
[0.1.0]: https://github.com/ravaninip/ChordMate/commits/main
