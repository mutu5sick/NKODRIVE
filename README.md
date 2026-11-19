# NEKODRIVE — JUCE VST3 starter project

Cat-inspired modern high-gain guitar amp plugin for Windows 11 and Cubase 14.
The voicing is a **Marshall-inspired starting point**, pushed toward modern high gain.

## Included in this starter

- VST3 and Standalone CMake targets
- Gain, Bass, Middle, Treble, Presence, Master
- Noise gate
- Cabinet voicing approximation (not yet a convolution IR loader)
- Compressor and reverb controls
- Stereo-only cross-channel delay doubler (automatically bypassed on mono input)
- Basic tuner frequency estimate and tuner mute
- Parameter state saved/restored by the DAW
- Simple preset XML export button
- Cat-themed custom GUI drawn in C++

## Important DSP status

This is a **buildable development starter**, not a finished commercial amp simulator.
The current amp uses a soft-clipping preamp and a simple tone-stack/cabinet approximation.
A true impulse-response loader, more accurate pitch-aware tuner, polished preset manager,
and final audio tuning are planned follow-up work. The stereo doubler currently uses
short cross-channel delays; it is not a pitch-shifting doubler.

## Requirements

- Windows 11 64-bit
- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.22 or newer
- Git
- Internet access on first configure (CMake downloads JUCE 8.0.6)
- Cubase 14 64-bit for host testing

## Build

Open **x64 Native Tools Command Prompt for VS 2022** or PowerShell:

```powershell
cd path\to\NEKODRIVE
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The VST3 bundle should be generated under:

```text
build\NEKODRIVE_artefacts\Release\VST3\NEKODRIVE.vst3
```

Copy the complete `NEKODRIVE.vst3` bundle directory to:

```text
C:\Program Files\Common Files\VST3\
```

Then in Cubase 14, open the Plug-in Manager and rescan if it does not appear.

## First test

1. Create an audio track and select your guitar input.
2. Insert NEKODRIVE as an insert effect.
3. Start with Gain 4–6, Bass 4–5, Middle 5–6, Treble 5, Presence 4–5.
4. Keep Master low at first; the plugin can produce high output levels.
5. Use a real guitar interface with an instrument/Hi-Z input.

## Notes

- The processor accepts matching mono-in/mono-out or stereo-in/stereo-out layouts.
- The doubler controls are disabled in the GUI when the processor is running mono.
- Use a low buffer size (64–128 samples) for comfortable playing, depending on your audio interface.
- This project has not been compiled or tested inside Cubase in this environment.


## GitHub Actions: build without installing Visual Studio locally

1. Create a new GitHub repository named `NEKODRIVE` (private or public).
2. Upload the contents of this project ZIP to the repository root (the `.github` folder must be included).
3. Open the repository's **Actions** tab and allow workflows if GitHub asks.
4. The `Build NEKODRIVE VST3 (Windows)` workflow runs on every push to `main`. You can also run it manually with **Run workflow**.
5. When the run finishes successfully, open that run and download the artifact named `NEKODRIVE-Windows-x64-VST3`.
6. Extract the ZIP. Copy the complete `NEKODRIVE.vst3` folder into:
   `C:\Program Files\Common Files\VST3\`
7. Restart Cubase 14 or rescan plug-ins in Plug-in Manager.

The GitHub workflow compiles the plugin on a Windows 2022 runner. A successful build confirms compilation and packaging, but final audio/host verification still needs Cubase 14 on a Windows PC.
