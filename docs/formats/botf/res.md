# Star Trek: Birth of the Federation: `stbof.res`

Status: **container understood**; the files inside are mostly not looked at
yet. Source: `scripts/botf_sample.py`'s zip listings of the English and
German `stbof.res`, October 2026.

`stbof.res` is an ordinary zip archive that Python's `zipfile` reads without
complaint, so `common/zip.cpp` should read it as it is. 6,013 entries (the
German one 6,014), no folders, names in lower case. 299 entries are stored
(method 0: all the `.gif` files) and the rest deflated (method 8).

## Contents, by extension

| Ext | Files | Total size | Examples | First guess |
|---|---|---|---|---|
| `.tga` | 4,864 | 215 MB | `asave.tga`, `asave_on.tga`, `oaccept1.tga` | interface graphics, with their text drawn in |
| `.wdf` | 377 | 468 KB | `options.wdf`, `tactical.wdf`, `gmesetup.wdf` | window layouts, one per screen or dialog |
| `.gif` | 299 | 20 MB | `t01_a.gif`, `bh01.gif` | textures of the `.hob` models (same base names) |
| `.hob` | 265 | 5 MB | `t01_a.hob`, `ch1_a.hob`, `cphaser.hob` | 3D models for ship combat; `_a`/`_b`/`_c` look like levels of detail |
| `.ani` | 65 | 13 MB | `gmel.ani`, `bnol.ani`/`bnom.ani`/`bnos.ani` | animations; `l`/`m`/`s` endings in three sizes (76,860 / 27,708 / 12,348 bytes) |
| `.cur` | 26 | 544 KB | `generic.cur`, `shipmove.cur`, `hwait.cur` | mouse cursors, some per race |
| `.tdb` | 22 | 339 KB | `cdiplomt.tdb`, `minor1.tdb`, `teachtip.tdb`, `treaty.tdb` | text: diplomacy per race, minor races, tutorial tips, intelligence reports |
| `.fnt` | 21 | 962 KB | `human15.fnt`, `klingn28.fnt`, `generic9.fnt` | fonts per race in 9, 15, 20 and 28 point |
| `.scr` | 20 | 3 KB | `ceogalli.scr` | end-of-game scripts, matching the CD's `*EOG*.wav` |
| `.bin` | 18 | 159 KB | `morale.bin`, `planboni.bin`, `shipname.bin`, `starname.bin`, `combat.bin`, `converse.bin` | rule and name tables |
| `.wtf` | 7 | 7 KB | `aiempire.wtf`, `aiminor.wtf`, `ccolval.wtf` | AI weights; colonisation values per race |
| `.tec` | 4 | 59 KB | `techtree.tec`, `techdesc.tec`, `tecfield.tec`, `racetech.tec` | technology tree and text |
| `.sst` | 4 | 70 KB | `shiplist.sst`, `shiptech.sst`, `shiprace.sst`, `shipdesc.sst` | ship classes |
| `.bst` | 3 | 127 KB | `edifice.bst`, `edifdesc.bst`, `edifbnft.bst` | buildings |
| `.rst` | 2 | 16 KB | `race.rst`, `racedesc.rst` | races |
| `.pst` | 1 | 85 KB | `planet.pst` | planets |
| `.est` | 1 | 780 B | `environ.est` | planet environments |
| `.lst` | 4 | 146 KB | `palette.lst`, `color.lst`, `texture.lst`, `keywords.lst` | |
| `.mnu` | 3 | 3 KB | `combat.mnu`, `fleets.mnu`, `system.mnu` | menus |
| `.vtx` | 2 | 10 KB | `laser.vtx`, `rlit.vtx` | combat effects |
| `.dic` | 1 | 82 KB | `lexicon.dic` | text |
| `.smt`, `.tfd` | 1 each | | `objstruc.smt`, `tfdesign.tfd` | |
| `.tbl` | 1 | 7 KB | `music.tbl` | binary, 7,323 bytes; starts with `u32` values all 0 or 1. Probably which of `music.snd`'s pieces may play in which situation |
| `.txt` | 1 | 8 KB | `credits.txt` | |

## Races

Per-race files use one letter: `c`, `f`, `h`, `k`, `r`. The fonts are named
`cardsn`, `ferngi`, `human`, `klingn` and `romuln`, so `h` is probably the
Federation (human) and `f` the Ferengi. The CD's movies (`C.avi`, `F.avi`,
`H.avi`, `K.avi`, `R.avi`) and end-of-game files use the same letters.

## English and German

The German archive has one more file (`rb_sbar.tga`), and 1,986 files
differ: 1,865 `.tga` (their text is part of the picture), 79 `.wdf`, all 22
`.tdb`, and the text-bearing tables (`.tec`, `.bst`, `.rst`, `.sst`,
`.pst`, `.est`, `.mnu`, `lexicon.dic`, `credits.txt`, `meplanet.bin` and `shipname.bin`).
Everything else, models and textures included, is shared.
