#!/bin/bash
# Yokoi (Game & Watch) on BananaOS: a Ports entry, /data/roms/ports/Yokoi.sh, with the app in /data/roms/ports/yokoi.
#
# BananaOS's menu has closed the panels, the buttons and the sound before this runs (docs/EMULATORS.md). Yokoi draws
# its two screens on both panels through BananaOS's SDL: the dummy video driver with SDL_BANANA_PANELS=side is one
# window over the two panels (the top one on the left), each half flipped straight onto its panel. The buttons are
# read by their kernel codes (never grabbed), the bottom touchscreen is BANANA_TOUCH's, and the volume keys stay with
# bananad. Settings, the last game and the save slots live in the app folder on the card.
#
#   Yokoi.sh [pack.ykp]       default: yokoi_pack_rgds.ykp in the app folder
set -u
base=$(CDPATH= cd -P "$(dirname "$0")" && pwd) || exit 1
app="$base/yokoi"
[ -d "$app" ] || app="$base"

# the log, readable on a computer: /data/banana/logs/yokoi/yokoi.log (.1: the run before)
logdir=/data/banana/logs/yokoi
mkdir -p "$logdir" 2>/dev/null || logdir="$app"
[ -f "$logdir/yokoi.log" ] && mv -f "$logdir/yokoi.log" "$logdir/yokoi.log.1"
exec > "$logdir/yokoi.log" 2>&1
printf 'Yokoi for BananaOS\n'; date

[ "$(uname -m)" = aarch64 ] || { echo "Yokoi needs ARM64 Linux (this is $(uname -m))"; exit 1; }
cd "$app" || exit 1
[ -f ./yokoi ] || { echo "No yokoi executable in $app"; exit 1; }
chmod +x ./yokoi 2>/dev/null

pack="yokoi_pack_rgds.ykp"
case "${1:-}" in *.ykp|*.YKP) pack=$1 ;; esac
[ -f "$pack" ] || echo "No game pack: $app/$pack (see README.txt); Yokoi shows how to add one"

export SDL_VIDEODRIVER=dummy SDL_BANANA_PANELS=side SDL_BANANA_TOP="${BANANA_TOP:-DSI-2}" SDL_AUDIODRIVER=alsa
export YOKOI_PLATFORM=banana YOKOI_TOUCH="${BANANA_TOUCH:-fe5e0000.i2c}"
# Ports' KMSDRM settings are for one-panel ports
unset SDL_KMSDRM_FIRST SDL_KMSDRM_REQUIRE_DRM_MASTER
[ -d "$app/lib" ] && export LD_LIBRARY_PATH="$app/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# BananaOS's Ports launcher darkens the bottom panel for one-panel ports (one-panel: /run/banana/panel-off names
# its backlight, and bananad keeps it dark across a wake-up while that file is there). Yokoi uses both panels: light
# it again; the launcher's own restore at the end does the same.
if [ -f /run/banana/panel-off ]; then
    bl=$(cat /run/banana/panel-off)
    rm -f /run/banana/panel-off
    [ -w "$bl/bl_power" ] && echo 0 > "$bl/bl_power" && echo "Bottom panel backlight on ($bl)"
fi

echo "Pack: $pack"
./yokoi "$pack"
status=$?
echo "Exit code: $status"
exit $status
