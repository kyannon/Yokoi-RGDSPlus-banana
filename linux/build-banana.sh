#!/bin/bash
# The BananaOS package: Yokoi for ARM64 and its Ports entry, as linux/build/banana/Yokoi_BananaOS_v<version>.zip,
# whose roms/ folder goes on the card's root (the BANANA partition, or \\<address>\banana over file sharing).
#
# Build on ARM64 Linux (an ARM64 Debian/Ubuntu with g++ and libsdl2-dev, or the CI's ubuntu-24.04-arm), or give
# linux/build.sh's cross variables (CXX, CPPFLAGS, SDL_CFLAGS, SDL_LIBS, LDFLAGS) for an ARM64 toolchain.
# libstdc++ and libgcc are linked in, so the binary needs only glibc and BananaOS's own SDL2 (/usr/lib).
set -eu
cd "$(dirname "$0")/.."
version=$(sed -n 's/.*Linux \([0-9][0-9.]*\).*/\1/p' linux/main.cpp | head -n 1)
out=linux/build/banana
stage=$out/stage
rm -rf "$stage"
mkdir -p "$out"
LDFLAGS="${LDFLAGS:-} -static-libstdc++ -static-libgcc" OUTPUT="$out/yokoi" bash linux/build.sh
if command -v file >/dev/null && ! file "$out/yokoi" | grep -q 'ARM aarch64'; then
    echo "Note: $out/yokoi is not an ARM64 binary; the package will not run on the handheld" >&2
fi

app=$stage/roms/ports/yokoi
mkdir -p "$app/licenses" "$stage/roms/ports/media/box"
install -m 755 banana/Yokoi.sh "$stage/roms/ports/Yokoi.sh"
install -m 755 "$out/yokoi" "$app/yokoi"
install -m 644 banana/README.txt "$app/README.txt"
install -m 644 assets/Yokoi.png "$stage/roms/ports/media/box/Yokoi.png"
install -m 644 NOTICES.md "$app/licenses/"
cp -R licenses/. "$app/licenses/"

zip="Yokoi_BananaOS_v$version.zip"
rm -f "$out/$zip"
if command -v zip >/dev/null; then
    (cd "$stage" && zip -qr -X "../$zip" roms)
else
    python3 - "$stage" "$out/$zip" <<'PY'
import os, sys, zipfile
stage, target = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as z:
    for root, dirs, files in os.walk(os.path.join(stage, "roms")):
        dirs.sort()
        for name in sorted(files):
            path = os.path.join(root, name)
            info = zipfile.ZipInfo(os.path.relpath(path, stage))
            info.external_attr = (os.stat(path).st_mode & 0o777) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            with open(path, "rb") as f:
                z.writestr(info, f.read())
PY
fi
echo "$out/$zip"
