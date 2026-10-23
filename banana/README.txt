Yokoi: Game & Watch emulator for BananaOS (RG DS / RG DS Plus)
================================================================

Install
-------
Copy the package's roms folder onto the card's root (the BANANA partition, or
\\<address>\banana over BananaOS's file sharing). That gives:

  roms/ports/Yokoi.sh              the entry in BananaOS's Ports shelf
  roms/ports/yokoi/yokoi           the emulator
  roms/ports/media/box/Yokoi.png   its picture on the shelf

Then put your own game pack here, beside the emulator:

  roms/ports/yokoi/yokoi_pack_rgds.ykp

No games are included. Make the pack yourself with the original Yokoi
conversion tools (CONVERT_ROM, "Android (RGDS)" target):

  python convert_original.py --target rgds
  python convert_3ds.py --target rgds

A pack made for the RG DS stock OS works as it is. The 3DS pack does not.

Open Ports > Yokoi in the menu.

Buttons
-------
  Library     D-pad          choose a game
              A / START      open it (in Watch mode)
              Y              load a save slot
              L1 / R1        previous / next initial
              L2 / R2        previous / next manufacturer
              X              advanced options (language)
              F or HOME      exit
  Watch       START          GAME A (or the single GAME button)
              SELECT         GAME B, where the machine has one
  Playing     the game's own buttons; L1 is TIME where a game has one
              L2 / R2        swap the screens
              F, L3, HOME    Pause (also START + SELECT)
  Pause       Up / Down, A   choose, confirm; B resumes
              Left / Right   save slot, segments, background colour

The volume keys and F + volume (brightness) are BananaOS's, as everywhere
in the system. Touch works on the bottom panel: swipe to browse, double tap
to open, tap in the menus.

Files
-----
Settings (language, display, the last game) and the save slots (saves/)
are kept in roms/ports/yokoi. The log of the last run is
banana/logs/yokoi/yokoi.log on the card (yokoi.log.1: the run before).
