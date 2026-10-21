# Star Trek: Birth of the Federation: `alt.res`

Status: **compression understood; not every image's header read yet.**
Source: the first 8 KB and last 4 KB of `alt.res` (`scripts/botf_sample.py`),
then the whole file decompressed by `scripts/botf_probe.py`, October 2026.

Not a zip, unlike `stbof.res`. All values little-endian.

```
offset 0   char[16] name     "alt.res", zero-padded
offset 16  u32                12,982,522: the compressed stream's length plus 4?
offset 20  u32                31
offset 24  LZSS stream
           zero padding to the end of the file (at least the last 4 KB)
```

The file is 12,992,536 bytes, so 9,994 bytes follow `20 + 12,982,522`;
656 of them aren't zero, so the stream may run into them.

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

Decompressing everything from offset 24 to the end of the file gives
29,773,558 bytes. 31 full-screen images of 960,018 bytes each make
29,760,558, so the count at offset 20 is the number of images, and 13,000
bytes are something else: headers or footers the plain images don't have,
or the padding at the end decoded as if it were data.

The images are uncompressed TGAs: type 2, no colour map, 800x600, 16-bit
(`A1R5G5B5`), descriptor `0x01` (one attribute bit, rows from the bottom
up), 960,018 bytes. Presumably full-screen backgrounds (an "alternate" set
to `stbof.res`'s).

Reading the whole stream as one, only some images decode cleanly. The
second probe run found:

- A plain TGA header only at 0. Headers seen by the first run further on
  (at 2,880,188, 3,840,206, 10,560,198) failed the stricter second check,
  so their colour-map fields hold garbage.
- 44 `TRUEVISION-XFILE.` footer signatures, each after 8 garbage bytes
  where a real footer has two offsets (usually 0), and in places the same
  footer repeated every 2,744 or 3,566 bytes.
- The 13,000 extra bytes match decoding the 9,994 bytes after the stream
  as if they were compressed.

The likely explanation: **each image is compressed on its own**, starting
with an empty window and a new flag byte, and the 31 streams are stored one
after the other. A decoder that carries on across a boundary uses leftover
flag bits on the next image's bytes and copies the previous image's data
where the encoder meant zeros, which is exactly what breaks headers and
footers, and copies old data around. So the images are probably TGA 2.0
files with a footer (and maybe an extension area), and their sizes are
not all 960,018 bytes.

`scripts/botf_probe.py` now reads the stream image by image three ways
(new window and flag byte per image, only one of the two, as in a test
built the same way) and shows the nonzero bytes after the stream, which
may be a table of names: 656 of the 9,994 bytes there aren't zero.
