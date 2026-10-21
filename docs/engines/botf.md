# Star Trek: Birth of the Federation: first look

Proposed engine id `botf`. Nothing is written yet; this page records what the
executables and the directory listings show, and how the engine should be
approached.

Sources (October 2026): `trek.exe` and `Trek.icd` from an English retail
install, and directory listings of that install and of the CD. Headers,
import tables, resources and relocations were read with a PE parser; nothing
was decrypted or run. The game's data files themselves haven't been looked at
yet.

## Identification

| File | Size | MD5 of first 5000 bytes |
|---|---|---|
| `trek.exe` | 237,547 | `a6e91db92cf3e0d6eac8e3ef666fa9b7` |
| `Trek.icd` | 1,771,520 | `c59c1835fc11b571482b7d330c068f9d` |

Detection should key on the data files (`stbof.res`, `english.snd`) once
they've been hashed, not only on these two: a patched or replaced executable
shouldn't stop a copy from being recognised.

## The executables

### `trek.exe` is only the SafeDisc loader

- Built 1999-03-22 with Microsoft's linker 5.10. It carries the SafeDisc
  signature `BoG_ *90.0&!!  Yy>` followed by the version 1, 6, 0 (SafeDisc
  1.06.000), and names `drvmgt.dll`, `Secdrv`, `wnaspi32.dll` and C-Dilla's
  splash bitmaps.
- Its imports are what a loader needs: `DeviceIoControl` (to the `secdrv`
  driver), `GetThreadContext`/`SetThreadContext`/`ResumeThread` (to start the
  game process and patch it), `GetDriveTypeA`/`GetLogicalDriveStringsA` (to
  find the CD). None of the game is in it.

### `Trek.icd` is the game, encrypted

- Built 1999-05-13. Linker version 2.18 and the section names `AUTO`,
  `DGROUP`, `.bss` are Watcom C/C++'s: a 32-bit Watcom build, as many
  MicroProse games of the time were.
- Code (`AUTO`, 1.5 MB) and initialised data (`DGROUP`, 172 KB) have an
  entropy of 7.99 and 7.92 bits per byte: they're encrypted. There are no
  readable strings in them.
- The import table, resources (one 32x32 icon, `TREK_ICON`) and relocation
  table are in the clear.
- **The 37,096 relocated dwords are in the clear too.** Every one of them
  holds an address inside the image, which encrypted bytes would almost
  never do: the encryption skips them so Windows can relocate the image
  before it's decrypted. This gives the addresses the code refers to (jump
  tables, virtual-function tables, globals, imported functions), but not the
  code or data around them. Too little to work from on its own.

So there is nothing to disassemble without first decrypting `Trek.icd`, which
needs the original CD and the SafeDisc driver (`secdrv.sys`), which current
Windows no longer loads. Circumventing the protection is a legal question
(anti-circumvention rules in copyright law, with interoperability exceptions
in some countries), and contributors need to settle it for their own
jurisdiction before doing it; CONTRIBUTING.md only allows reverse engineering
"where your local law permits it". **The engine shouldn't depend on it.** The
other sources CONTRIBUTING.md allows are enough to start: the data files,
observation of the running game, and public format documentation. BotF has
an active modding community that has documented most of its data files over
two decades (check where each piece of knowledge came from before using it).

Worth checking: whether the official 1.0.2 update replaces `trek.exe`
with an unprotected executable. Fan tools patch the game's code at fixed
file offsets, which only works on an unencrypted binary, so one may exist
legitimately.

### What the imports say about the game

| DLL | Imports | Meaning for Reluva |
|---|---|---|
| `mpr97.dll` | `MPRInitialize`, `MPRCreateSurface`, `MPRBlt`, `MPRBltTransparent`, `MPRBltFill`, `MPRSetChromaKey`, `MPRLockSurface`, `MPRGetBackbuffer`, `MPRGetDeviceResolution`... plus 16 by ordinal | MicroProse's own rendering layer (the install also has `mpr555*.dll`, `mpr565*.dll` and `gen.mpr`). Pixel-format variants for 15-bit (555) and 16-bit (565) colour: the game draws in hi-colour, 2D with colour-keyed sprites. The ordinal-only imports are probably the 3D side used by ship combat |
| `mss32.dll` | 25 `AIL_*` sample functions, including `AIL_load_sample_buffer` / `AIL_sample_buffer_ready` and `AIL_set_sample_file` | Miles Sound System, digital samples only (no MIDI). Double-buffered sample playback means music and speech are streamed. `common/audio`'s mixer covers this |
| `MSVFW32.dll` | `ICLocate`, `ICDecompress`, `ICSendMessage` | Video for Windows codecs: the `.avi` movies are decoded through an installed codec. The codec (FourCC in each AVI) decides which decoder Reluva needs |
| `DPLAYX.dll` | ordinals 1 and 4 | DirectPlay (`DirectPlayCreate`, `DirectPlayLobbyCreateA`): multiplayer. Last, if ever |
| `ole32.dll` | `CoCreateInstance`, `CoInitialize` | COM, for DirectX |
| `WINMM.dll` | `timeGetTime`, `timeBeginPeriod`... | Timing |
| `ADVAPI32.dll` | `RegOpenKeyExA`, `RegEnumValueA` | Reads settings (install and CD paths) from the registry |
| `KERNEL32.dll` | `GetDriveTypeA`, `CreateFileA`, `FindFirstFileA`, threads... | Finds the CD; plain file access, so data paths are relative names that Reluva's case-insensitive lookup can resolve |

## Files

### Install folder

- Data: `stbof.res`, `alt.res` (archives), `english.snd`, `sfx.snd` (sound
  archives), `intro.wav`, `Stbof.ini` (settings).
- Saves: `game0.sav`-`game11.sav`, `Auto.sav`, `Last.sav`. The player's own
  saves are the best source for working out the game state.
- Not needed by Reluva: `trek.exe`, `Trek.icd`, the MPR DLLs and `gen.mpr`,
  Miles (`Mss32.dll`, `Mss16.dll`, `mssb16.tsk`), `Msvcrt.dll`, `dxreg.dll`,
  `Msdaemon.dll`, `mprtest.exe`, `mpsinfo.exe`/`Mpsinfo.txt` (system info),
  `colormod.exe`, `hsbr.exe`, `Uninst.isu`, `msnzone.url`, `Crash.log`,
  `disk.id`, `readme.doc`, `tutor1.pdf`.

### CD

- `music.snd`: the music. Not copied by the install, so it's read from the CD.
- `ENGLISH/` and `GERMAN/`: per language `STBOF.RES`, the speech archive
  (`english.snd`, `german.snd`), the movies (`open.avi` and one per race:
  `C`, `F`, `H`, `K`, `R`, presumably Cardassian, Federation, Ferengi,
  Klingon, Romulan), `INTRO.WAV`, and end-of-game narration
  `<race>EOG<ending>.wav` with the endings `Alli`, `Anni`, `Domi` and `Loss`
  (by their names: alliance, annihilation, domination, defeat). `GERMAN/`
  also has `game0.sav`-`game3.sav`.
- Also on the CD root: `alt.res`, `sfx.snd`, `lang.dat`, `os.dat`,
  `test.wav`, `stbof.BMP`.
- SafeDisc (`00000001.TMP`, `clcd16.dll`, `clcd32.dll`, `clokspl.exe`,
  `drvmgt.dll`, `secdrv.sys`, `dplayerx.dll`, `.016`/`.256` splash files),
  InstallShield (`data1.cab`, `_sys1.cab`, `_user1.cab`, `setup.*`, `SETUPDIR`),
  DirectX 6 (`redist`), `AUTOPLAY.EXE`, `ar*.exe`: not needed.

Reluva should accept the install folder with the CD's files beside it (or a
second folder for the CD), and say clearly when `music.snd` or the movies
are missing rather than refuse to start.

## Plan

The New Worlds engine's order fits here too: readers and a data viewer first,
then the game screens, then the rules, with recordings of the original filling
what the data can't show.

1. **Detection** (`reluva --hash`): `stbof.res`, `english.snd`, `trek.exe`;
   English and German.
2. **Archives.** Check whether `stbof.res` and `alt.res` are zip files
   (`reluva-tool hexdump` for `PK\x03\x04`; the modding community treats them
   as such). If so, `common/zip.cpp` already reads them. Work out the `.snd`
   containers' index (WAV entries) and write `formats/botf/snd.md`.
3. **Data viewer**, as for New Worlds: images (TGA is in `common/image.cpp`),
   sounds, music, the text tables, and the binary tables in `stbof.res`
   (races, ship classes, buildings, tech tree, star and planet types). Each
   format written up under `docs/formats/botf/` with how it was worked out.
4. **Movies.** Read the AVIs' FourCC; add an AVI reader and the decoder
   they use to `common/`.
5. **Saves.** Decode `.sav` by comparing saves taken a turn apart in the
   running game. This is the map of the game state the rules act on.
6. **Screens.** The galaxy map, system, empire and diplomacy screens, built
   from the window layouts in the data and screenshots of the original at
   its fixed resolution, drawn in 16-bit colour as MPR did.
7. **Rules.** Turn processing (production, research, growth, morale,
   AI) and ship combat, from the data tables, documentation, and
   measurements in the running game, checked against saves. This is
   by far the largest part, as it is for any 4X game.

Running the original to record it needs Windows 98/XP in a VM with the
CD (SafeDisc 1 works there), or an unprotected executable if one is found.
