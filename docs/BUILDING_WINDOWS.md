# Building and installing ChordMate on Windows

ChordMate runs on Windows as a VST® 3 plug-in. There are two ways to get it. **Option A** needs no tools at all; **option B** builds the
plugin on your own PC.

## Option A: download a prebuilt plugin

1. Open the [Releases](https://github.com/ravaninip/ChordMate/releases) page and download
   `ChordMate-windows-<version>.zip`.
   For the very latest development build, open the [Actions](https://github.com/ravaninip/ChordMate/actions)
   tab, pick the most recent green *Build ChordMate* run and download the **ChordMate-windows** artifact
   (requires a GitHub account).
2. Extract the zip and continue with [Install](#install).

## Option B: build on your PC

Requirements: 64-bit Windows 10/11, an internet connection, about 8 GB of free disk space.

1. Clone the repository into a **short** path, for example `C:\dev\ChordMate`:
   ```powershell
   git clone https://github.com/ravaninip/ChordMate.git C:\dev\ChordMate
   ```
2. Open **PowerShell** (administrator rights are not required) and install the tools once:
   ```powershell
   winget install Kitware.CMake
   winget install Git.Git
   winget install Microsoft.VisualStudio.2022.BuildTools --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
   ```
   The last command downloads several GB. Skip it if you already have Visual Studio 2022 with the
   *Desktop development with C++* workload.
3. **Close and reopen PowerShell** so that `PATH` is refreshed.
4. Build and install with a single command:
   ```powershell
   cd C:\dev\ChordMate
   powershell -ExecutionPolicy Bypass -File .\build-windows.ps1
   ```
   The script checks the prerequisites, downloads JUCE, builds, runs the engine tests (they must report
   0 failures) and copies the plugin to:
   - `C:\Program Files\Common Files\VST3` when PowerShell runs as administrator;
   - `%LOCALAPPDATA%\Programs\Common\VST3` otherwise.

   Options: `-SkipTests` skips the engine tests, `-InstallDir <path>` sets the destination folder.

The first build takes 5–15 minutes.

## Install

Copy the whole `ChordMate.vst3` folder to `C:\Program Files\Common Files\VST3` (administrator rights required).
If you used option B, the script has already done this.

## Enable the plugin in Live 12

1. **Settings → Plug-Ins**: enable *Use VST3 Plug-In System Folders* and press **Rescan**
   (hold **Alt** for a full rescan).
2. If the plugin was installed to `%LOCALAPPDATA%`, add that folder as a *VST3 Plug-In Custom Folder*.
3. **ChordMate** appears under Plug-Ins (instruments). Drag it onto a new MIDI track.

## Troubleshooting

| Symptom | Fix |
|---|---|
| `cmake` is not recognised | Reopen PowerShell after `winget install`, or restart the PC. |
| `No CMAKE_CXX_COMPILER could be found` | The C++ workload is missing: *Visual Studio Installer → Modify → Desktop development with C++*. |
| Errors about paths being too long | Move the project to `C:\dev\ChordMate` and delete the `build` folder. |
| Downloading JUCE fails | Check your internet/corporate proxy and retry (`git` must be able to reach github.com). |
| Live does not see the plugin | Rescan with Alt held; check the VST3 folders in Settings; make sure `ChordMate.vst3\Contents\x86_64-win\ChordMate.vst3` exists. |
| Live reports the plugin as invalid or crashes | [Open an issue](https://github.com/ravaninip/ChordMate/issues/new/choose) and attach `Log.txt` from `%APPDATA%\Ableton\Live 12.x\Preferences\` plus the error message. |
| SmartScreen or antivirus blocks the file | The plugin is not code-signed yet: *More info → Run anyway*, or add an exception. |

---

VST is a registered trademark of Steinberg Media Technologies GmbH.
