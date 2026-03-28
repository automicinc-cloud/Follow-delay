# Follow Delay — Build & Install Guide

## What This Is

A **VST3 delay plugin** for REAPER that bends delay time in response to your playing. Plug in your guitar via MIDI trigger, draw a motion curve, and the delay follows your performance timing.

---

## Prerequisites

1. **JUCE Framework** (v7.x or v8.x)
   - Download from https://juce.com/download
   - Unzip somewhere (e.g. `C:\JUCE` or `~/JUCE`)

2. **CMake** (3.22+)
   - https://cmake.org/download

3. **C++ Compiler**
   - Windows: Visual Studio 2022 (Community is free)
   - macOS: Xcode 14+
   - Linux: GCC 12+ or Clang 15+

---

## Build Steps

### Option A: CMake (Recommended)

```bash
cd FollowDelay

# Point to your JUCE install
cmake -B build -DJUCE_DIR=/path/to/JUCE

# Build Release
cmake --build build --config Release
```

The built VST3 will be in:
- **Windows:** `build/FollowDelay_artefacts/Release/VST3/Follow Delay.vst3`
- **macOS:** `build/FollowDelay_artefacts/Release/VST3/Follow Delay.vst3`

With `COPY_PLUGIN_AFTER_BUILD` enabled in the CMake file, it also copies automatically to your system VST3 folder.

### Option B: Projucer

1. Open the Projucer (ships with JUCE)
2. Create a new Audio Plug-in project named "Follow Delay"
3. Add all files from `Source/` to the project
4. Set: VST3 = enabled, Plugin accepts MIDI = yes
5. Export & build in your IDE

---

## Install in REAPER

1. Copy the `.vst3` file (or folder) to your VST3 path:
   - **Windows:** `C:\Program Files\Common Files\VST3\`
   - **macOS:** `/Library/Audio/Plug-Ins/VST3/`
   - **Linux:** `~/.vst3/`

2. Open REAPER → Options → Preferences → Plug-ins → VST → Re-scan

3. Insert on a track: **FX → Follow Delay**

---

## Guitar Setup in REAPER

Follow Delay needs **audio input** (your guitar) and **MIDI trigger input** to know when to fire.

### Method 1: MIDI Trigger via a Separate Track

1. **Track 1 — Guitar audio:** arm for recording, input = your audio interface
2. **Track 2 — MIDI trigger:** arm for recording, input = your MIDI controller/keyboard
3. Route Track 2's MIDI output → Track 1 (drag Track 2's route button to Track 1)
4. Put Follow Delay on Track 1

Now when you hit a MIDI note (default: C4 / note 60), the delay motion fires on your guitar audio.

### Method 2: Audio + MIDI on Same Track

If your interface sends both audio and MIDI:
1. Create one track
2. Set input to your audio interface (guitar)
3. Also enable MIDI input on the same track
4. Insert Follow Delay

### Method 3: Use "Any Held Note" Trigger

If you don't have a MIDI controller, change **Trigger Source** to "Any Held Note" and use a virtual keyboard or MIDI item in REAPER.

---

## Quick Start After Install

1. Insert Follow Delay on your guitar track
2. Make sure MIDI is reaching the plugin (check the trigger indicator)
3. The default curve is a gentle descending sweep — play a note on your MIDI controller and strum your guitar
4. Tweak **Start** (where the bend begins) and **End** (where it lands)
5. Draw your own curve by clicking in the motion graph
6. Adjust **Follow Amount** to control how much the effect adapts to your note length

---

## Controls Quick Reference

| Control | What It Does |
|---------|-------------|
| Start / End | Delay time range in ms — motion sweeps between these |
| Warp | Changes the speed curve of the motion (ease-in vs ease-out) |
| Feedback | How many repeats |
| Tone | Brightness of repeats |
| Mix | Dry/wet blend |
| Fractal | Adds organic irregularity to the motion |
| Smoothness | How rounded vs jagged the curve is |
| Stereo | Offsets L/R motion timing for width |
| Follow Amount | How much the effect adapts to your actual note length |
| Learn | Capture a pitch-bend gesture from your playing |
| Follow Timing | ON = adapts to your note length. OFF = fixed duration |
| Retrigger | What happens if you trigger again mid-motion |

---

## Troubleshooting

- **No sound from the delay:** Check Mix is above 0, and that audio is passing through the track
- **Trigger not firing:** Ensure MIDI is reaching the plugin. Check Trigger Note matches what you're sending
- **Clicks/pops:** Turn on Safe Mode (Advanced panel) and reduce Feedback
- **REAPER doesn't see the plugin:** Re-scan VST3 folder in Preferences
