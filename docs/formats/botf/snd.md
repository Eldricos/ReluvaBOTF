# Star Trek: Birth of the Federation: `.snd` sound archives

Status: **understood**, except what `flags` 2 means in `sfx.snd`. Source: the first 8 KB and last 4 KB of
`sfx.snd`, `english.snd` and `music.snd` (`scripts/botf_sample.py`), then
every directory entry of the whole files with the first bytes of each sound
(`scripts/botf_probe.py`), October 2026.

All values little-endian. None of the archives stores names: the game
refers to sounds by their number. Every entry is 16 bytes:

```
u32 offset     from the start of the file
u32 size       bytes stored
u32 third      sfx.snd: always 0x0040CA9A (left over from the packing tool?);
               english.snd, music.snd: the decoded size, see below
u32 flags      sfx.snd: 0 or 2; english.snd: 0; music.snd: 3
```

## `sfx.snd` and `english.snd`

```
u32 directory offset
sounds
directory: entries to the end of the file
```

| File | Directory at | Entries | Sounds are |
|---|---|---|---|
| `sfx.snd` | 7,745,170 | 163 | complete RIFF WAVE files, all PCM, mono, 22,050 Hz, 16-bit |
| `english.snd` | 39,487,492 | 1,364 | bare mono IMA ADPCM blocks, no WAV header |

In `sfx.snd` the WAV files follow each other with no gaps, from offset 4
to the directory. 142 entries have `flags` 0 and 21 have 2; the WAV files
are alike either way, so 2 says something about how the game plays them
(looping?).

## `music.snd`

```
16 times: u32 count, u32 offset    a list of `count` entries at `offset`
segments, from offset 128
the 16 lists, near the end of the file
```

Counts 5, 15, 15, 15, 16, 15, 10, 10, 9, 12, 11, 4, 4, 3, 3, 4: 151
segments in 16 pieces. The lists follow each other directly (each offset is
the previous one plus 16 x count), from 75,447,424 to the end of the file.
`music.tbl` in `stbof.res` probably says which piece plays when (see
[res.md](res.md)).

## The ADPCM data

`english.snd` and `music.snd` hold IMA ADPCM in the block layout of WAV
format 0x11 (as New Worlds uses, `common/audio/wav.cpp`), without the WAV
header:

- `english.snd`: mono, 512-byte blocks. Each starts with a 4-byte header
  (`s16` first sample, `u8` step index 0-88, `u8` 0), then 508 bytes of
  4-bit samples: 1,017 samples per block.
- `music.snd` (flags 3): stereo, 1,024-byte blocks, an 8-byte header (one
  per channel), then the channels' samples interleaved 4 bytes at a time.

Found by the headers repeating every 512 (english) and 1,024 (music)
bytes from each sound's start: a plausible sample, a step index under 89,
then a zero byte. Checked on all 1,364 speech clips and 151 music segments:
each starts with a valid header, and every stored size is a whole number of
blocks.

The sample rate isn't stored. It's 22,050 Hz, like `sfx.snd`'s WAV files:
listening in Reluva's viewer at 11,025, 22,050 and 44,100 Hz, speech and
music sound right only at 22,050 (October 2026).

`third` is the size of the decoded sound as a 16-bit PCM WAV file: a
44-byte header plus 2 bytes per sample and channel. For example 62,464
bytes = 122 blocks hold at most 124,074 samples, and `third` = 248,166 =
44 + 2 x 124,061. In all 1,515 sounds `third` - 44 divides evenly by 2 x
channels and leaves between 4 and 1,038 samples of the last block unused,
so it gives each sound's exact length: decode the blocks, then keep
(`third` - 44) / (2 x channels) samples.
