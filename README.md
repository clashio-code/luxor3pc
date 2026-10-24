# Luxor 3 Native Track Prototype — RG40XXV / muOS

This is the **next engineering milestone** for a native ARM64 reimplementation. It does not run `Luxor3.exe`, and it is not yet the complete game.

## What it does
- Uses SDL2 + OpenGL ES 2.0 (no Box64 or Wine in this prototype).
- Parses the real text track `assets/path.LX3TRK` extracted from the supplied Luxor 3 archive.
- Draws the track sections from the game's own `Verts` and `Sections` blocks.
- Animates a test marble along the selected section.
- Left/right on keyboard or D-pad selects a section; Escape/Start exits.
- Logs graphics driver and parsing results.

## Build for ARM64
The supplied device report indicates the RG40XXV has runtime SDL2/GLES libraries but no compiler or development headers, so build on a PC/CI runner. Upload this folder to a GitHub repository and run **Actions → Build Luxor 3 native track prototype (ARM64) → Run workflow**. Download the `Luxor3_Native_Track_ARM64` artifact.

**Compatibility warning:** the CI binary is a first ABI test, not guaranteed to match every muOS image. If SDL2 cannot initialize on-device, the log will identify that. The target runtime must provide compatible `libSDL2` and `libGLESv2` libraries. Do not overwrite any existing Luxor port; test in a separate folder first.

## Device test
1. Extract the package into a new folder on the SD card.
2. Make sure `run.sh` is executable (if copying through Windows, use the port's usual launcher method).
3. Launch it. Read `luxor3_native_track.log` if it exits.

Expected log lines include `[track] loaded ... vertices, ... sections` and `[GL] renderer=... version=...`.

## Important next blockers
1. Confirm the binary launches and GLES works on the actual RG40XXV.
2. Parse `.lvl` and `#include` data so the correct track/map is selected per level.
3. Render actual sprite/PNG assets and create a ball-train/shooter prototype.
4. Reconstruct matching, collisions, score, UI, effects, audio, and level progression.

This is a track renderer/input smoke test using real Luxor 3 track data—not a finished game port.
