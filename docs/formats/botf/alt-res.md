# Star Trek: Birth of the Federation: `alt.res`

Status: **compression understood; layout after the first image unconfirmed.**
Source: the first 8 KB and last 4 KB of `alt.res` (`scripts/botf_sample.py`),
October 2026. `scripts/botf_probe.py` decompresses the whole file and lists
what it holds, to check the rest.

Not a zip, unlike `stbof.res`. All values little-endian.

```
offset 0   char[16] name     "alt.res", zero-padded
offset 16  u32                12,982,522: the compressed stream's length plus 4?
offset 20  u32                31
offset 24  LZSS stream
           zero padding to the end of the file (at least the last 4 KB)
```

The file is 12,992,536 bytes, so 9,994 bytes follow `20 + 12,982,522`.
Whether the stream ends there, and whether 31 is the number of images, is
what the probe checks.

## Compression

A variant of the common 1989 LZSS (Haruhiko Okumura's), with the reference
bytes in a different order:

- A 4,096-byte window, all zeros at the start; writing starts at `0xFEE`
  (4,096 - 18).
- A flag byte describes the next eight items, low bit first. Bit 1: one
  literal byte. Bit 0: a two-byte reference `a b`:
  - length = `(a >> 4) + 2` (2 to 17 bytes)
  - window position = `((a & 0x0F) << 8) | b`
- Every byte output, literal or copied, is also written to the window at
  the current position, which then advances (wrapping at 4,096).

How it was worked out:

- The bytes from offset 24 show a flag byte (`e7`) followed by eight items,
  then another flag byte (`07`) after exactly eight, and the literals
  include `20 03 58 02 10 01`: 800, 600, 16 and 1, the end of a TGA header.
  With references read as length `(a >> 4) + 2`, the first two references
  fill exactly the 9 header bytes between `00 00 02` and the width; the
  usual Okumura order (length in `b`'s low nibble) gives 12.
- Long runs of references `f0 1b`, `f0 2c`, `f0 3d`, ... each copy 17 bytes
  from a position 17 further on: zero runs, copied from the not yet written
  part of the window. With writing starting at `0xFEE`, each of these 161
  references reads 18 bytes ahead of the write position, as Okumura's
  encoder does when it matches the zero-filled start of the window; any
  other start position puts them at odd distances or reads pixels instead
  of zeros.
- Decoded this way, the first 8 KB give a valid TGA header and 37 rows of
  pixels, mostly black with scattered single grey and coloured pixels (a
  starfield), every pixel aligned on 2 bytes.

## Contents

The stream starts with an uncompressed TGA: type 2, no colour map, 800x600,
16-bit (`A1R5G5B5`), descriptor `0x01` (one attribute bit, rows from the
bottom up). That is 960,018 bytes; 31 of them would be about 29.8 MB, which
fits the 13 MB stream for mostly black pictures. Presumably full-screen
backgrounds (the "alternate" set to `stbof.res`'s).
