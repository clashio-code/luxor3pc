#!/bin/sh
# muOS / RG40XXV launcher; logs to the port folder.
set -u
HERE="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
cd "$HERE"
LOG="$HERE/luxor3_native_track.log"
exec >"$LOG" 2>&1
echo "=== Luxor 3 native track prototype ==="
echo "date=$(date)"
echo "device=$(uname -a)"
if [ ! -x "$HERE/luxor3_native_track" ]; then echo "ERROR: binary missing; compile for muOS ARM64 first."; exit 127; fi
export SDL_GAMECONTROLLERCONFIG="${SDL_GAMECONTROLLERCONFIG:-}"
exec "$HERE/luxor3_native_track" "$HERE/assets/path.LX3TRK"
