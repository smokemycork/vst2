# Tension Builder - tech house buildup VST3 / AU

One big **BUILD** knob (automate it in Ableton) drives everything at once:

- rising resonant high-pass filter
- tempo-synced stutter gate that speeds up 1/4 -> 1/8 -> 1/16 -> 1/32 as tension rises
- rising band-passed noise riser (stereo)
- growing reverb wash (the tail rings out into the drop)
- drive and stereo widening
- **Impact**: sub boom + click on the drop

## Using it in Ableton Live
Put it on a drum bus, a group, or the master (it is an insert effect, stereo).

1. **Manual:** draw an automation ramp on *Build* from 0 to 100% over 8 or 16 bars.
2. **Auto:** set *Auto Build Length* to 4/8/16/32 bars. Build then ramps with the timeline and
   fires the impact on the bar the phrase wraps (the drop). Needs the transport playing; when stopped, the Build knob is used.
3. **The drop:** hold the DROP button, or drop the Build automation from high to ~0 instantly.
   Everything snaps off, the Impact hits, and the reverb tail rings out.
4. Pick a preset, tweak *Curve* (how front/back-loaded the rise is) and the per-effect amounts.

## Getting the installers (no compiler needed)
1. Create a free GitHub account and a new empty repository.
2. Upload this whole folder to it (including the hidden `.github` folder).
3. Open the **Actions** tab -> **Build installers** -> **Run workflow**. Takes about 10 minutes.
4. Download `TensionBuilder-Windows` (Setup .exe) and/or `TensionBuilder-macOS` (.pkg) from the finished run.

Windows installs to `C:\Program Files\Common Files\VST3`. In Live: Preferences -> Plug-Ins -> enable VST3 system folders -> Rescan.
macOS installs VST3 + AU system-wide. The installer is unsigned: right-click the .pkg -> Open the first time.

## Building locally
Requires CMake 3.22+ and a C++17 compiler (Visual Studio 2022 or Xcode):

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

Then run the Inno Setup script (Windows) or `installer/macos/make_pkg.sh` (macOS).
