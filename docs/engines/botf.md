# Star Trek: Birth of the Federation

Engine id `botf`, in `src/engines/botf/` (CMake option `RELUVA_ENGINE_BOTF`,
on by default). This page records what the executables, the directory
listings and the data files show, how the engine is approached, and where it
stands.

## Where it stands

| Part | State |
|---|---|
| Detection | English retail by `stbof.res` and `english.snd`; German by `stbof.res` (as on the CD) and a `german.snd` of any hash; anything else with `stbof.res` and `sfx.snd` as "unknown release" |
| Finding files | `game_files.cpp`: the game folder, its language folder, and `Stbof.ini`'s `CDPATH` with its language folder, so a minimum install finds `music.snd` on the CD or copied beside the game |
| `stbof.res` | read with `common/zip.cpp` |
| `alt.res` | `AltResArchive` (`alt_res.cpp`): directory and LZSS ([alt-res.md](../formats/botf/alt-res.md)); all 31 images checked in the viewer, after correcting the window start |
| `.snd` archives | `SndArchive` (`snd_archive.cpp`): both layouts; WAV entries through `common/audio/wav.cpp`, bare IMA ADPCM through its `decodeImaAdpcm` at 22,050 Hz (confirmed by ear) ([snd.md](../formats/botf/snd.md)) |
| Fonts | `Font` (`font.cpp`): the 120 glyphs of every `.fnt`, found by their frames, with their advances; drawing UTF-8 text; all 21 load ([fnt.md](../formats/botf/fnt.md)) |
| Text tables | `TextTable` (`text_table.cpp`): every `.tdb` string and its key, Windows-1252 decoded to UTF-8 ([tdb.md](../formats/botf/tdb.md)) |
| Data tables | `DataTable` (`data_table.cpp`) with a `TableLayout` per table: races, ships, technologies, research fields, buildings, planets, environments, star and ship names, ships' requirements and races, planet values, with names, pictures, models and descriptions; ships' weapons, shields, hit points, range and costs, and buildings' costs, maintenance and output, matched against a recording of the game; many numbers still unnamed ([tables.md](../formats/botf/tables.md)) |
| Viewer | Images (`stbof.res` and `alt.res` TGAs, full-screen ones at their own size), Fonts (glyph sheet and sample text), Sounds (effects, speech, music pieces), Text (each table's strings, wrapped, in a game font) and Data (records, known fields, the bytes not worked out, descriptions) pages; keys in the README |
| Tools | `reluva-tool alt-list` (list and extract `alt.res`), `snd-list` (list and decode a `.snd` archive to WAV) |
| Tests | `tests/test_botf.cpp`: LZSS (with bytes from the retail file's start), `alt.res`, fonts and line wrapping, text and data tables, both `.snd` layouts, ADPCM without a WAV header, `Stbof.ini` and file finding |
| Not yet | `.gif` textures (stb_image is built without GIF), how the game colours text, what the text tables' keys mean, most of the data tables' numbers, the other data tables, models, movies, saves, the game itself |

Sources (October 2026): `trek.exe` and `Trek.icd` from an English retail
install, directory listings of that install and of the CD, and
`scripts/botf_sample.py`'s report on the data files (sizes, hashes, first and
last bytes, zip listings). Headers, import tables, resources and relocations
were read with a PE parser; nothing was decrypted or run.

## Identification

| File | Size | MD5 of first 5000 bytes |
|---|---|---|
| `trek.exe` | 237,547 | `a6e91db92cf3e0d6eac8e3ef666fa9b7` |
| `Trek.icd` | 1,771,520 | `c59c1835fc11b571482b7d330c068f9d` |

Detection should key on the data files (`stbof.res`, `english.snd`; hashes
in [install.md](../formats/botf/install.md)), not on these two: a patched or
replaced executable shouldn't stop a copy from being recognised.

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
| `MSVFW32.dll` | `ICLocate`, `ICDecompress`, `ICSendMessage` | Video for Windows codecs: the `.avi` movies are decoded through an installed codec. They all turn out to be Cinepak (`cvid`), 800x300 |
| `DPLAYX.dll` | ordinals 1 and 4 | DirectPlay (`DirectPlayCreate`, `DirectPlayLobbyCreateA`): multiplayer. Last, if ever |
| `ole32.dll` | `CoCreateInstance`, `CoInitialize` | COM, for DirectX |
| `WINMM.dll` | `timeGetTime`, `timeBeginPeriod`... | Timing |
| `ADVAPI32.dll` | `RegOpenKeyExA`, `RegEnumValueA` | Reads settings (install and CD paths) from the registry |
| `KERNEL32.dll` | `GetDriveTypeA`, `CreateFileA`, `FindFirstFileA`, threads... | Finds the CD; plain file access, so data paths are relative names that Reluva's case-insensitive lookup can resolve |

## Files

Where everything is and what it's for: [install.md](../formats/botf/install.md).
In short:

| Data | Format | Reluva has |
|---|---|---|
| `stbof.res`: interface graphics, models, layouts, tables, text | plain zip ([res.md](../formats/botf/res.md)) | `common/zip.cpp` |
| `alt.res`: 31 full-screen pictures | own header, LZSS-compressed 800x600 16-bit TGAs ([alt-res.md](../formats/botf/alt-res.md)) | TGA decoding; LZSS to write |
| `sfx.snd`, `english.snd`, `music.snd` | numbered entries: WAV files, or bare IMA ADPCM blocks ([snd.md](../formats/botf/snd.md)) | IMA ADPCM inside `common/audio/wav.cpp` |
| movies on the CD | AVI, Cinepak video | nothing yet: an AVI reader and a Cinepak decoder |
| `intro.wav`, `*EOG*.wav` | plain WAV | `common/audio/wav.cpp` |

`music.snd` and the movies are on the CD only (a "minimum" install).

## Plan

The New Worlds engine's order fits here too: readers and a data viewer first,
then the game screens, then the rules, with recordings of the original filling
what the data can't show.

1. **Detection.** Done for English; hash `german.snd` when a German install
   is available.
2. **Archives.** Done: `stbof.res`, `alt.res` and the `.snd` archives (see
   the table above).
3. **Data viewer**, as for New Worlds. Done: interface TGAs, `alt.res`
   pictures, fonts, sounds, speech, music, text tables, and the data tables'
   layouts. Next: the numbers in the data tables, and the tables not started
   (`.pst`, `.est`, `.bin`, `.wtf`, the rest of `.sst` and `.tec`). Each format
   written up under `docs/formats/botf/` with how it was worked out.
4. **Models.** `.hob` with their `.gif` textures, on the Models page with
   `common/gfx3d`.
5. **Movies.** An AVI reader and a Cinepak decoder in `common/`.
6. **Saves.** Decode `.sav` by comparing saves taken a turn apart in the
   running game. This is the map of the game state the rules act on.
7. **Screens.** The galaxy map, system, empire and diplomacy screens, from
   the `.wdf` window layouts and screenshots of the original, at 800x600 in
   16-bit colour as MPR drew them.
8. **Rules.** Turn processing (production, research, growth, morale, AI)
   and ship combat, from the data tables, documentation, and measurements in
   the running game, checked against saves. This is by far the largest
   part, as it is for any 4X game.

Running the original to record it needs Windows 98/XP in a VM with the
CD (SafeDisc 1 works there), or an unprotected executable if one is found.
