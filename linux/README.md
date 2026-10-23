# Yokoi Linux / RGDS / RGDS+ — v1.4.2

Native C++20/SDL2 frontend based on Yokoi revision `f273025`. No Android APK, JNI or RetroArch is required. The shared code under `android/app/src/main/cpp` supplies CPU/input utilities only.

## Build

On Debian/Ubuntu install `g++` and `libsdl2-dev`, then run from the repository root:

```sh
bash linux/build.sh
```

Output: `linux/build/yokoi`, for the host architecture. For RGDS/RGDS+, build on ARM64 or supply `CXX`, `CPPFLAGS` (sysroot), `SDL_CFLAGS`, `SDL_LIBS`, `LDFLAGS` and `OUTPUT` for an ARM64 toolchain and ARM64 SDL2 development files. `MAIN_SOURCE` selects a test source instead of `linux/main.cpp`.

The release is built with GCC 13.3 and SDL2 2.30 headers. Highest required symbols: GLIBC_2.38 and GLIBCXX_3.4.31. SDL is loaded from the console runtime. No SDL_image or SDL_ttf is needed: PNG decoding is bundled LodePNG and the UI font is embedded DejaVu Sans with its notice.

## Runtime and installation

The update archive supplies:

- `Roms/APPS/Yokoi.sh`
- `Roms/APPS/Yokoi_RGDSPlus/yokoi`
- `Roms/APPS/Imgs/Yokoi.png`

Keep the existing working `lib` directory, pack, configuration and `saves`. These are not distributed in the update. The same directory and launcher serve both console models. Fresh installations need compatible ARM64 runtime libraries as well as a user-created pack; this is not a self-contained first-install package.

The pack path defaults to `yokoi_pack_rgds.ykp` beside the executable; a path can also be passed as the first argument. The loader accepts RGDS/Android packs (platform 2, pack format v2/v3, content version >=3), not 3DS packs. See the root README for original conversion instructions.

The launcher selects the Stock Wayland/ALSA environment and writes `Yokoi_Arranque.txt`. The app creates two independent SDL windows and uses calibrated Anbernic input profiles. L2/R2 changes manufacturer in the library and swaps game screens during play. Touch swipe/double tap is supported.

## Setup input and game state

`native_setup.h` forwards GAME A/B to the existing virtual-input mapping. It does not inspect lives, Game Over state, ROM hashes or inactivity. GAME A/B machines retain that native behaviour. `single_game_gate.h` additionally guards only mappings marked `single_game_button`: Balloon Fight, Climber and Super Mario Bros., both variants of each. START produces a bounded GAME pulse; SELECT is unmapped. Fresh CEND entries rearm GAME after native game end. Press eligibility is recorded before the Pause chord grace so an in-game press cannot be queued across Game Over. Holding through Game Over requires release and a new press. The guard is recreated from the existing CPU on resume/load without changing save formats; explicit Pause GAME actions remain intentional new games.

The library-open button must be released first. A short grace reserves START+SELECT for Pause; short taps receive a minimum native input duration. Explicit Pause GAME A/B actions use a native setup pulse after initialization. A physical setup request stops Watch-only host-clock refresh so active game state is not overwritten.

START maps to GAME A or the sole GAME button. SELECT maps to GAME B only where implemented by the original input configuration. There is no Climber SELECT-to-START alias. Both variants of Balloon Fight, Climber and Super Mario Bros. use START only.

Watch initialization and refresh use upstream per-title local-time addresses. Loaded saves and active games are not continuously modified to match host time. `last-game.cfg` atomically stores the last opened ROM reference and manufacturer. Saves retain their existing format.

## Display, language, audio and volume

Pause contains Resume, Choose another game, Save, Load, Manage slots, GAME A, GAME B, Segments, Background colour and Exit (translated in PT). Display changes apply without a CPU reset and are saved in `display.cfg`; the lower panel previews the chosen game. Advanced options contains Language, Configure buttons and Back. English is the default for new settings; existing PT/EN preferences remain valid.

SDL audio negotiates sample format, frequency and channels, with a stereo fallback and conversion for the obtained device format. Physical volume uses `0.35 × (level/10)²`. The app and launcher restore entry volume on exit; existing crash/signal cleanup is retained. Runtime libraries and firmware are not modified.

Black game margins preserve aspect ratio in the renderer. The separate RGDS fullscreen/black-bars report remains deferred.

## Checks

`--version` prints version without starting SDL. `--smoke-test PACK` renders a few library frames. `--test-game PACK INDEX` opens a selected game in Watch. Use `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy` for headless UI tests; `YOKOI_TEST_DUAL=1` enables two test windows on one display.

- `tests/single_game_gate.cpp`: full CPU snapshot comparisons on all six external ROMs, active reset reproduction, native end/replay, held/late presses and saved-state resume.
- `tests/native_setup.cpp`: opening hold, fresh/repeated setup presses, short taps, Pause chord and absence of SELECT-to-START alias.
- `tests/single_game_input.cpp`: verifies actual Balloon Fight/Climber/Super Mario Bros. input mappings from an external pack: SELECT does not press GAME; START does.
- `tests/physical_start_ui.cpp`: virtual-controller library entry, native START/SELECT forwarding, Pause and resume without recreating the CPU.
- `tests/state_roundtrip.cpp`: save/load/replay and rollback after corrupted files across six real games.
- `tests/synthetic_pack.py`: generates original synthetic test fixtures, with no commercial ROM data.
- `tests/native_rom_compare.cpp`: reproducible RAM/LCD comparison with and without native setup presses, using an external pack.

Other tests cover display settings, clock, language, calibration, library return, screen swaps, audio and volume. See `VALIDATION_v1.4.2.md` for recorded results and console feedback. Do not distribute test ROM packs, game artwork, personal saves or screenshots.

## v1.4.2 input fixes

TF_104 GAME A/B wiring is corrected in virtual_input.h. juggler_gate.h applies only to MB_108, observing its native MISS/Watch display and protecting both setup buttons. Tests: juggler_tropical.cpp and physical_juggler_ui.cpp. SingleGameGate remains unchanged.

## BananaOS

BananaOS (github.com/JorreFog/BananaLinux) has no compositor, no Wayland and no `anbernic_misc` nodes. Its emulator contract (`docs/EMULATORS.md` there) and its SDL2 2.32 patch (`buildroot/patches/sdl2/0001-banana-panels.patch`) decide how Yokoi runs. `banana/Yokoi.sh` sets `YOKOI_PLATFORM=banana`, and the frontend then:

- **Display**: SDL's dummy video driver with `SDL_BANANA_PANELS=side` is one display over both panels (top on the left), whose window framebuffer BananaOS's SDL page-flips straight onto the two panels. `banana_platform.h` lays Yokoi's two 1024x768 screens into that window's halves (a viewport and scale per panel; 640x480 panels on the RG DS, 1024x768 on the Plus) and presents once per frame. A full-window clear is replaced by a fill of the panel's viewport. The software renderer gets its textures mirrored once at load instead of a flipped copy per segment (pixel-identical; also used by any software renderer elsewhere), and while a game runs with nothing over it, frames whose lit segments are unchanged are not redrawn.
- **Buttons**: the joypad is read by kernel codes, never grabbed (bananad reads the same device): A `BTN_EAST`, B `BTN_SOUTH`, X `BTN_NORTH`, Y `BTN_WEST`, START, SELECT, L1 `BTN_TL` (TIME / previous initial), R1 next initial, L2/R2 screen swap and manufacturer. F (`BTN_MODE`, on release, unless F + volume was used for brightness), L3 and HOME (`BTN_Z`, adc-keys) are Pause / Exit. No SDL game controller, mapping or calibration is used there.
- **Touch**: the bottom panel's touchscreen is the event device whose sysfs path contains `BANANA_TOUCH` (default `fe5e0000.i2c`), read raw as on the stock OS.
- **Volume**: bananad owns the volume keys and the mixer. Yokoi keeps a fixed software level (`YOKOI_VOLUME`, 0..10, default 10 = gain 0.35) and shows no overlay. Audio is ALSA `default`.
- **Lifecycle**: banana-ui has released the panels, input and sound before the launcher runs. Sleep (SIGSTOP/SIGCONT) is absorbed by the worker's existing wall-clock reset, Watch mode resyncs the clock, and SIGTERM (shutdown while playing) exits cleanly. Settings and save slots stay in the app folder.

The Ports launcher darkens the bottom panel for one-panel ports (`/run/banana/panel-off`); `Yokoi.sh` turns it back on. It writes `/data/banana/logs/yokoi/yokoi.log`.

Build on ARM64: `bash linux/build-banana.sh` writes `linux/build/banana/Yokoi_BananaOS_v<version>.zip` (libstdc++/libgcc static; needs only glibc and BananaOS's `/usr/lib/libSDL2-2.0.so.0`; BananaOS 0.4 uses Bootlin's 2025.08 glibc toolchain, newer than Ubuntu 24.04's glibc 2.39). `.github/workflows/bananaos.yml` builds it on `ubuntu-24.04-arm`, runs the headless tests and uploads the package.

Headless checks without the panels: `YOKOI_SHARED_WINDOW=side|stack` and `YOKOI_SHARED_SIZE=WxH` give the same shared layout on plain SDL. `tests/banana_platform.cpp` checks the layout, touch mapping, pad codes and F/L3/HOME handling, and drives a full session (manufacturers, Watch, Pause, resume, F + volume, library return, exit) through the pad's key codes.

Not yet confirmed on the handhelds: the build has been tested headless only (x86-64 against BananaOS's patched SDL 2.32.10, dummy video/audio). DRM presentation, the touchscreen match and audio need a first run on an RG DS / RG DS Plus.
