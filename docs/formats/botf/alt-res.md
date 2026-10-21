# Star Trek: Birth of the Federation: `alt.res`

Status: **understood**, except the 16-bit check value. Source: the first
8 KB and last 4 KB of `alt.res` (`scripts/botf_sample.py`), then four
runs of `scripts/botf_probe.py` on the whole file, October 2026. The last
run decoded all 31 files by their directory entries: each gives exactly
its decoded size, from exactly its compressed size, and starts with a
valid TGA header. Those runs used a wrong window start (corrected below,
after the images showed garbled in Reluva); sizes and headers don't depend
on it, but the pixels, footers and check-value tests did.

An archive of 31 TGA images, each LZSS-compressed. Not a zip, unlike
`stbof.res`. All values little-endian.

```
offset 0   char[16] "alt.res", zero-padded
offset 16  u32      directory offset (12,982,522)
offset 20  u32      number of files (31)
offset 24  the files' compressed data, one after the other
           directory: 40 bytes per file
           zero padding to the end of the file (8,774 bytes)
```

A directory entry:

```
char[16] name       zero-padded, e.g. "cicon800.tga"
u32 offset          of the compressed data, from the start of the file
u32 index           0 to 30, the entry's own number
u32 decoded size    960,018 or 960,044
u32 compressed size
u32 flag            1 in every entry (compressed?)
u32 check           under 65,536; how it's computed is still open
```

The check isn't the sum, the sum or XOR of 16-bit words, either half of a
CRC-32, nor six common CRC-16s of the compressed bytes. The same tests on
the decoded bytes were made with the wrong window start, so they need
redoing. A reader doesn't need it.

The data isn't stored in directory order: the last entry, `rw.tga`, comes
first at offset 24, then entries 0 to 29 in order. Each file's compressed
data ends exactly where the next one's begins, and the last ends at the
directory.

## Compression

A variant of the common 1989 LZSS (Haruhiko Okumura's), with the reference
bytes in a different order:

- Each file is compressed on its own: a 4,096-byte window, all zeros at
  the start, writing from position **1**, and a new flag byte.
- A flag byte describes the next eight items, low bit first. Bit 1: one
  literal byte. Bit 0: a two-byte reference `a b`:
  - length = `(a >> 4) + 2` (2 to 17 bytes)
  - window position = `((a & 0x0F) << 8) | b`
- Every byte output, literal or copied, is also written to the window at
  the current position, which then advances (wrapping at 4,096).
- A file ends when its decoded size (from the directory) is reached; any
  flag bits left in the last flag byte are unused, and the next file's
  data starts at the next byte.

How it was worked out:

- The bytes from offset 24 show a flag byte (`e7`) followed by eight items,
  then another flag byte (`07`) after exactly eight, and the literals
  include `20 03 58 02 10 01`: 800, 600, 16 and 1, the end of a TGA header.
  With references read as length `(a >> 4) + 2`, the first two references
  fill exactly the 9 header bytes between `00 00 02` and the width; the
  usual Okumura order (length in `b`'s low nibble) gives 12.
- Long runs of references `f0 1b`, `f0 2c`, `f0 3d`, ... each copy 17
  bytes from a position 17 further on: runs of one byte value (black),
  each copied from the byte just before the write position. That pins the
  write position to 1 more than each reference's position, so writing
  starts at 1. (The first version of this page started it at `0xFEE`,
  which makes these references read 18 bytes *ahead*, into the zero-filled
  window, as Okumura's encoder can. That decodes the header and the black
  runs the same way, but every other copy then reads from the wrong place:
  the images came out garbled after their first rows.)
- Checked on the first 37 rows of `rw.tga` (the first 8 KB): with writing
  from 1, the copies that aren't runs are at even distances (2-byte pixels)
  in 96% of cases, against 4% with `0xFEE`; the pixels use 96 colours, the
  most common dim blue, green, red and grey, none with the top bit set: a
  starfield of single stars on black. Other start positions give streaks
  of saturated colour and 180-310 colours.
- Decoding the whole file as one stream broke every image after the first:
  headers with garbage colour-map fields, and copies of old data where
  zeros belong. That is what a decoder does if it keeps the window and the
  flag bits across a file boundary, which led to decoding file by file:
  the second file's header then decodes cleanly, and the compressed data
  ends where the directory says the next file starts.

## Contents

Every file is an uncompressed TGA: type 2, no colour map, 800x600, 16-bit
(`A1R5G5B5`), descriptor `0x01` (one attribute bit, rows from the bottom
up). 960,018 bytes, or 960,044 for the 21 with 26 more bytes after the pixels,
presumably a TGA 2.0 footer (`TRUEVISION-XFILE.`). The probe found the
signature in 20 of them, with nonzero offsets before it and none in
`h_fo.tga`, but it decoded with the wrong window start, so those details
need checking again. A reader should ignore anything after the pixels, as
`common/image.cpp` already does.

The names follow the race letters (`c`, `f`, `h`, `k`, `r`, see
[res.md](res.md)):

| Per race | Names |
|---|---|
| all five | `?icon800.tga`, `?l.tga`, `?w.tga`, `?_ss.tga`, `?_sbg01.tga` |
| one race only | `c_oohq.tga`, `f_toc.tga`, `h_fchq.tga`, `h_fo.tga`, `k_gh.tga`, `r_tshq.tga` |

Entry 0 is `c_sbg01.tga`. Presumably full-screen backgrounds in
each race's style, an "alternate" set to `stbof.res`'s (which screens use
them is for the running game to show).
