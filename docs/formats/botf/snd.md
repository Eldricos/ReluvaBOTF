# Star Trek: Birth of the Federation: `.snd` sound archives

Status: **directories understood; the meaning of two fields unconfirmed.**
Source: the first 8 KB and last 4 KB of `sfx.snd`, `english.snd` and
`music.snd` (`scripts/botf_sample.py`), October 2026.
`scripts/botf_probe.py` lists every entry of the whole files to check the rest.

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
| `sfx.snd` | 7,745,170 | 163 | complete RIFF WAVE files (the first: PCM, mono, 22,050 Hz, 16-bit) |
| `english.snd` | 39,487,492 | 1,364 | bare mono IMA ADPCM blocks, no WAV header |

What `flags` 2 means in `sfx.snd` is open (looping? ADPCM?).

## `music.snd`

```
16 times: u32 count, u32 offset    a list of `count` entries at `offset`
segments, from offset 128
the 16 lists, near the end of the file
```

Counts 5, 15, 15, 15, 16, 15, 10, 10, 9, 12, 11, 4, 4, 3, 3, 4: 151
segments in 16 pieces. The lists follow each other directly (each offset is
the previous one plus 16 x count). `music.tbl` in `stbof.res` probably says
which piece plays when. 1,280 bytes follow the last list, not looked at.

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
then a zero byte. Every stored size is a whole number of blocks.

The sample rate isn't stored; 22,050 Hz (like `sfx.snd`'s first WAV) is the
first thing to try, by ear.

`third` is then just under the size of the decoded sound as a 16-bit PCM
WAV file: subtracting a 44-byte WAV header leaves at most what the blocks
hold (for example 62,464 bytes = 122 blocks = at most 124,074 samples;
`third` = 248,166 = 44 + 2 x 124,061). If so, it gives each sound's exact
length.
