# Star Trek: Birth of the Federation: install layout

Source: directory listings of an English retail install ("minimum" install)
and of its CD, and `scripts/botf_sample.py`'s report on them (sizes, hashes,
first and last bytes, the zip listings), October 2026.

## Identification

| File | Size | MD5 of first 5000 bytes |
|---|---|---|
| `stbof.res` (install; `ENGLISH/STBOF.RES` on the CD is the same) | 79,901,501 | `2e888c1eaefa901da9e2dbd6229798a5` |
| `GERMAN/STBOF.RES` (CD) | 79,385,299 | `21df64c3479690c70f50bfc0bed3792b` |
| `english.snd` (install and `ENGLISH/` on the CD) | 39,509,316 | `806bf768303d60745c792fae58ec506f` |
| `sfx.snd` (install and CD) | 7,747,778 | `ee602d2849a2731c789101ec5ea2e238` |
| `music.snd` (CD only) | 75,449,840 | `8c6b02cc653054fdcab78db463e5f06a` |
| `alt.res` (install and CD) | 12,992,536 | `85b83104c882c4e54c2084a1779e0d6e` |
| `trek.exe` | 237,547 | `a6e91db92cf3e0d6eac8e3ef666fa9b7` |
| `Trek.icd` | 1,771,520 | `c59c1835fc11b571482b7d330c068f9d` |

The install's copies are byte-for-byte the CD's (same size and hash). An
English install is detected by `stbof.res` and `english.snd`; a German one
would have `german.snd` (not hashed yet).

## Game data

| File | Where | What |
|---|---|---|
| `stbof.res` | install; CD per language | zip archive, 6,013 files: graphics, models, layouts, data tables ([res.md](res.md)) |
| `alt.res` | install, CD | 31 full-screen 800x600 16-bit TGAs, LZSS-compressed ([alt-res.md](alt-res.md)) |
| `sfx.snd` | install, CD | sound effects: 163 WAV files ([snd.md](snd.md)) |
| `english.snd` | install; CD `ENGLISH/` | speech: 1,364 IMA ADPCM clips ([snd.md](snd.md)) |
| `music.snd` | CD only | music: 16 pieces in 151 stereo IMA ADPCM segments ([snd.md](snd.md)) |
| `intro.wav`, `ENGLISH/INTRO.WAV` | install, CD | narration, a plain RIFF WAVE |
| `ENGLISH/*EOG*.wav` | CD | end-of-game narration per race and ending (`Alli`, `Anni`, `Domi`, `Loss`), with `*eog*.scr` in `stbof.res` |
| `ENGLISH/*.avi` | CD only | `open.avi` and one per race (`C`, `F`, `H`, `K`, `R`): Cinepak (`cvid`) video, 800x300; the race movies also have an audio stream |
| `Stbof.ini` | install | settings, below |
| `*.sav` | install | saved games |

### `Stbof.ini`

Plain `KEY=VALUE` lines, no section:

```
CDPATH=D:\
HDPATH=C:\botf\
CURRENTGAME=0
LANGUAGE=ENGLISH
MOVIEON=ON
SOUNDON=ON
MUSICVOLUME=100
FXVOLUME=100
VIDEOVOLUME=127
VOICEVOLUME=100
MONO=YES
STARTINGSEED=0
INSTALL=MINIMUM
DIRECT X NOT 6.0 INSTALLED BY BOTF
```

`INSTALL=MINIMUM` and `CDPATH` are why `music.snd` and the movies are read
from the CD. `LANGUAGE` names the CD folder (`ENGLISH`, `GERMAN`). Reluva
should look for the CD's files beside the install's, or in a second folder,
and run without music or movies if they're missing.

## Not needed by Reluva

- Install: `trek.exe` and `Trek.icd` (SafeDisc loader and the encrypted game,
  see [../../engines/botf.md](../../engines/botf.md)), MicroProse's renderer
  (`mpr*.dll`, `gen.mpr`), Miles Sound System (`Mss32.dll`, `Mss16.dll`,
  `mssb16.tsk`), `Msvcrt.dll`, `dxreg.dll`, `Msdaemon.dll`, `mprtest.exe`,
  `mpsinfo.exe` and `Mpsinfo.txt`, `colormod.exe`, `hsbr.exe`, `Uninst.isu`,
  `msnzone.url`, `Crash.log`, `disk.id`, `readme.doc`, `tutor1.pdf`.
- CD: SafeDisc (`00000001.TMP`, `clcd16.dll`, `clcd32.dll`, `clokspl.exe`,
  `drvmgt.dll`, `secdrv.sys`, `dplayerx.dll`, `*.016`, `*.256`),
  InstallShield (`data1.*`, `_sys1.*`, `_user1.*`, `_INST32I.EX_`,
  `_ISDel.exe`, `setup.*`, `layout.bin`, `SETUPDIR`, and `lang.dat` and
  `os.dat`, its language and platform lists), DirectX 6 (`redist`,
  `DX6INST.DLL`, `dx6inst.exe`, `dxregtst.exe`), `AUTOPLAY.EXE`,
  `autorun.inf`, `ar*.exe`, `test.wav`, `stbof.BMP`, `trek.ico`.
