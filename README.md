# ChordMate

[![Build](https://github.com/ravaninip/ChordMate/actions/workflows/build.yml/badge.svg)](https://github.com/ravaninip/ChordMate/actions/workflows/build.yml)
[![License: AGPL v3](https://img.shields.io/badge/license-AGPL--3.0-blue.svg)](LICENSE)
![Formats](https://img.shields.io/badge/formats-VST3%20%7C%20AU-informational)
![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20macOS-lightgrey)

**ChordMate is an arranging assistant plugin for Ableton Live 12.** It suggests chords, progressions and
melodies using music theory, arranges them for ensembles from piano to full orchestra, and lets you drag
the result straight into Live as MIDI clips.

Everything runs offline and is deterministic: no network, no AI models, no accounts.

> [!NOTE]
> The plugin interface is currently in Italian. An English UI is planned (see [Roadmap](#roadmap)).

## Contents

- [Features](#features)
- [Installation](#installation)
- [Using ChordMate in Live](#using-chordmate-in-live)
- [Building from source](#building-from-source)
- [Project layout](#project-layout)
- [Known limitations](#known-limitations)
- [Roadmap](#roadmap)
- [Contributing](#contributing)
- [License](#license)

## Features

- **Keys and scales** – 11 scales and modes: major, natural/harmonic/melodic minor, church modes, pentatonics, blues.
- **Genre profiles** – 11 macro-genres (Universal, Pop, Rock, Jazz, R&B/Neo-soul, Electronic, Hip-hop/Lo-fi,
  Cinematic, Classical, Latin/Bossa, Ambient). A genre sets roman-numeral presets, suggestion weights and colour
  chords (♭VII in rock, ii–V and tritone subs in jazz, chromatic mediants in cinematic), chord colour, rhythm
  style, bass figure, melody rhythm/swing, ensemble and polyharmony in one go.
- **Next-chord suggestions** – the six most sensible follow-ups to the selected chord, each with its roman numeral
  and the reason (authentic/plagal/deceptive cadence, modal borrowing, secondary dominant, tritone substitution…).
  An *Adventure* slider controls how many non-diatonic options appear.
- **Progressions** – genre presets or seeded random generation; **Reharmonize** swaps chords for functional
  equivalents, secondary dominants or tritone subs while keeping the first and last chord.
- **Voice leading** – inversions are chosen to minimise movement. Styles: block chords, 1/16 arpeggio,
  1/8 up-down arpeggio, 1/8 pulse, off-beat; optional bass line (eighths, walking, syncopated, bossa, octaves).
- **Macro-controlled melody** – *Energy*, *Motion* (steps ↔ leaps), *Tension* (chord tones ↔ appoggiaturas,
  9ths/13ths, chromaticism), *Contour* (free, arch, rising, falling, wave) and *Register*. Melodies are built in
  4-bar phrases (motif, transposed repetition, variation, cadence); the controls reshape the same melody.
- **Orchestral arranging and polyharmony** – piano/pad, string quartet, string orchestra, symphony orchestra
  (strings, horns, low brass, timpani, woodwinds), SATB choir, brass ensemble. Four-part writing keeps thirds and
  sevenths and avoids parallel fifths/octaves. Polyharmony adds in-scale extensions or upper-structure triads
  (e.g. G/F = Fmaj7(9,♯11)). The **Arrangement .mid** export is multitrack: drop it below Live's tracks to get one
  track per section.
- **MIDI capture** – record incoming MIDI, quantise (1/4–1/32 with strength), snap wrong notes to the scale, and
  **harmonise** the captured melody (Viterbi search over fit + transition scores).
- **Live detection** – the chord you are holding and the most likely key (Krumhansl profiles).
- **Session recall** – progression, melody, capture, settings and window size are saved with the Live set.
- **Resizable UI** (60–200 %), and recording continues while the plugin window is closed.

## Installation

1. Download the latest zip for your platform from [**Releases**](https://github.com/ravaninip/ChordMate/releases)
   (or the `ChordMate-windows` / `ChordMate-macos` artifact of a recent run in the
   [Actions](https://github.com/ravaninip/ChordMate/actions) tab).
2. Copy the plugin bundle to your plug-in folder:
   - **Windows:** `ChordMate.vst3` → `C:\Program Files\Common Files\VST3`
   - **macOS:** `ChordMate.vst3` → `~/Library/Audio/Plug-Ins/VST3`, or `ChordMate.component` → `~/Library/Audio/Plug-Ins/Components`
3. In Live: **Settings → Plug-Ins**, enable the system VST3 folder (and/or Audio Units) and press **Rescan**
   (hold **Alt/Option** for a full rescan).

> [!WARNING]
> The binaries are not code-signed yet. On Windows, SmartScreen may ask for confirmation
> (*More info → Run anyway*). On macOS, remove the quarantine flag if the plugin is blocked:
> `xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/ChordMate.vst3`

## Using ChordMate in Live

VST/AU plugins **cannot send MIDI to other tracks** in Live (a Live limitation, not a ChordMate one), so the
workflow is:

| Direction | How |
|---|---|
| Live → ChordMate | On the ChordMate track set **MIDI From** to the track you want analysed (Monitor: *In*). ChordMate detects chord and key and can record the notes. |
| ChordMate → Live | **Drag** the *Chords / Melody / Capture / Arrangement .mid* tiles onto a MIDI track: a clip is created. A single click opens *Save as*. |

ChordMate loads as an instrument with a small built-in synth for previewing. Leave *play incoming MIDI*
off if you don't want to hear the input.

**Quick start** (current Italian UI labels in brackets)

1. Pick key, scale and genre, press *Apply preset* [*Applica preset*] or *Generate random* [*Genera casuale*],
   then *New melody* [*Nuova melodia*]. *Play all* [*▶ Ascolta tutto*] previews it.
2. Drag *Chords .mid* [*Accordi*] and *Melody .mid* [*Melodia*] onto **other** MIDI tracks with real instruments.
3. To work from an existing part, route it with *MIDI From*, press *Record* [*● Registra*], then
   *Harmonize → chords* [*Armonizza → accordi*].

## Building from source

Requirements: CMake ≥ 3.22 and a C++17 compiler (Visual Studio 2022 on Windows, Xcode on macOS, GCC/Clang on
Linux). JUCE is fetched automatically from GitHub at configure time.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure   # engine tests (21,000+ checks)
```

The plugin ends up in `build/ChordMate_artefacts/Release/VST3/ChordMate.vst3` (and `AU/` on macOS).

| CMake option | Default | Description |
|---|---|---|
| `CHORDMATE_BUILD_PLUGIN` | `ON` | Build the JUCE plugin (downloads JUCE). Turn off to build and test only the engine. |
| `CHORDMATE_BUILD_TESTS` | `ON` | Build the theory-engine tests. |
| `CHORDMATE_BUILD_PLUGIN_TESTS` | `OFF` | Host-less smoke test of processor and editor; can save UI screenshots. |
| `CHORDMATE_COPY_AFTER_BUILD` | `OFF` | Copy the plugin into the user's VST3/AU folder after building. |
| `CHORDMATE_JUCE_PATH` | *(empty)* | Use a local JUCE checkout instead of downloading it. |
| `CHORDMATE_JUCE_TAG` | `8.0.15` | JUCE tag to download. |

Plugin smoke test (no host needed, writes two UI screenshots):

```bash
cmake -B build -DCHORDMATE_BUILD_PLUGIN_TESTS=ON
cmake --build build --target chordmate_plugin_smoke
./build/chordmate_plugin_smoke screenshot.png
```

**Windows users:** see [docs/BUILDING_WINDOWS.md](docs/BUILDING_WINDOWS.md) for a step-by-step guide and the
one-command `build-windows.ps1` script.

## Project layout

```
Source/Engine/     pure C++ theory engine (no JUCE, unit-testable)
  Theory.*         scales, chords, suggestions, progressions, voicing, bass, melody, quantisation, harmonisation
  Style.*          genres, roman-numeral presets, polychords, SATB voice leading, ensemble arranging
  MidiWriter.*     Standard MIDI File writer (480 PPQ, tempo and time signature)
  Project.*        full plugin state and text serialisation
Source/            JUCE plugin (processor, editor, preview synth, widgets)
Tests/             engine tests and plugin smoke test
docs/              additional documentation
```

## Known limitations

- Preview plays at the host tempo (or 120 BPM) but is not locked to the transport position.
- Recording follows the host position while Live is playing, otherwise an internal clock: for accurate results
  record with Live playing and the metronome on.
- Clips imported from MIDI files take their length from the end of the track (rounded to the bar); adjust the
  clip loop if Live shows a different length.
- The UI is tested in CI builds and the smoke test; please report anything that behaves differently in your host.

## Roadmap

- English user interface (and translatable strings)
- Code-signed and notarised installers for Windows and macOS
- Transport-synced preview
- CLAP format and testing in more DAWs

Ideas and votes are welcome in [Issues](https://github.com/ravaninip/ChordMate/issues).

## Contributing

Bug reports, feature ideas and pull requests are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md) and the
[Code of Conduct](CODE_OF_CONDUCT.md) first. Security issues: see [SECURITY.md](SECURITY.md).

## License

ChordMate is free software licensed under the [GNU Affero General Public License v3.0](LICENSE).

It is built with [JUCE](https://juce.com) (used under the AGPLv3) and the Steinberg VST3 SDK.
VST is a registered trademark of Steinberg Media Technologies GmbH. Ableton and Live are trademarks of
Ableton AG. ChordMate is not affiliated with or endorsed by either company.
