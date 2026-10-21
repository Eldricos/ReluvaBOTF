#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Reluva contributors
"""Check the layouts worked out from botf_sample.py's samples against whole files.

Reads (never writes) Star Trek: Birth of the Federation's install folder,
and optionally the CD, and writes a text report about:

- alt.res: decompresses the LZSS stream (docs/formats/botf/alt-res.md) file
  by file in three ways, lists the TGA files found, and shows the bytes
  after the stream
- sfx.snd, english.snd, music.snd: every directory entry, with the first
  bytes of each sound (docs/formats/botf/snd.md)
- music.tbl from stbof.res: its first lines

The report holds no images or sounds, only headers, sizes and offsets.
Only standard-library Python 3.8+ is needed:

    python3 botf_probe.py "C:\\Program Files\\MicroProse\\Trek" --cd D:\\ -o botf-probe.txt

Decompressing alt.res takes up to a few minutes.
"""

import argparse
import re
import struct
import sys
import time
import zipfile
from pathlib import Path


def find(folder, name):
    """Case-insensitive lookup of `name` in `folder`."""
    if folder is None or not Path(folder).is_dir():
        return None
    for p in Path(folder).iterdir():
        if p.name.lower() == name.lower() and p.is_file():
            return p
    return None


def tga_header(data, off):
    """(width, height, bits, descriptor, size, type, id length, colour map length)
    if a TGA header plausibly starts at `off`; size is None for RLE types."""
    if off + 18 > len(data):
        return None
    id_len, cmap_type, img_type = data[off], data[off + 1], data[off + 2]
    if cmap_type > 1 or img_type not in (1, 2, 3, 9, 10, 11):
        return None
    cmap_first, cmap_len, cmap_bits = struct.unpack_from("<HHB", data, off + 3)
    if cmap_type == 0 and (cmap_len or cmap_bits):
        return None
    if cmap_type == 1 and (cmap_len == 0 or cmap_bits not in (15, 16, 24, 32)):
        return None
    x0, y0, w, h, bits, desc = struct.unpack_from("<HHHHBB", data, off + 8)
    if not (8 <= w <= 4096 and 8 <= h <= 4096 and bits in (8, 15, 16, 24, 32)):
        return None
    if x0 > 4096 or y0 > 4096 or desc & 0xC0:
        return None
    cmap_size = cmap_len * ((cmap_bits + 7) // 8) if cmap_type else 0
    size = None
    if img_type in (1, 2, 3):
        size = 18 + id_len + cmap_size + w * h * ((bits + 7) // 8)
    return w, h, bits, desc, size, img_type, id_len, cmap_len


def describe_tga(h):
    w, hgt, bits, desc, size, img_type, id_len, cmap_len = h
    return (f"TGA type {img_type} {w}x{hgt} {bits}-bit, descriptor {desc:#04x}, "
            f"id length {id_len}, colour map {cmap_len}, "
            + (f"{size:,} bytes" if size else "RLE"))


class Lzss:
    """alt.res's LZSS decoder, with its state kept between calls so the
    stream can be read file by file (see decode_files)."""

    def __init__(self, src, pos):
        self.src = src
        self.pos = pos
        self.flags = 0
        self.reset_window()

    def reset_window(self):
        self.window = bytearray(4096)
        self.r = 0xFEE

    def new_flags(self):
        self.flags = 0

    def read(self, count, out):
        """Append `count` bytes to `out`; False if the input ran out first."""
        src, window = self.src, self.window
        want = len(out) + count
        while len(out) < want:
            self.flags >>= 1
            if not self.flags & 0x100:
                if self.pos >= len(src):
                    return False
                self.flags = src[self.pos] | 0xFF00
                self.pos += 1
            if self.flags & 1:
                if self.pos >= len(src):
                    return False
                c = src[self.pos]
                self.pos += 1
                out.append(c)
                window[self.r] = c
                self.r = (self.r + 1) & 0xFFF
            else:
                if self.pos + 1 >= len(src):
                    return False
                a, b = src[self.pos], src[self.pos + 1]
                self.pos += 2
                at = ((a & 0x0F) << 8) | b
                for k in range(((a >> 4) & 0x0F) + 2):
                    c = window[(at + k) & 0xFFF]
                    out.append(c)
                    window[self.r] = c
                    self.r = (self.r + 1) & 0xFFF
        return True


def decode_files(data, count, reset_window, new_flags, report, label):
    """Decode `count` TGA files one after another from offset 24, resetting the
    window and/or starting a new flag byte at each file. Returns the files."""
    lz = Lzss(data, 24)
    files = []
    for n in range(count):
        if reset_window:
            lz.reset_window()
        if new_flags:
            lz.new_flags()
        start_in = lz.pos
        out = bytearray()
        if not lz.read(18, out):
            report.append(f"   [{label}] file {n}: input ran out at {lz.pos:,}")
            break
        h = tga_header(out, 0)
        if not h or not h[4]:
            report.append(f"   [{label}] file {n} at input {start_in:,}: no TGA header: "
                          f"{out.hex(' ')}")
            break
        lz.read(h[4] - 18, out)
        files.append((start_in, out, h))
    report.append(f"   [{label}] {len(files)} files read; input position after the "
                  f"last: {lz.pos:,}")
    return files, lz.pos


def probe_alt(path, report):
    data = path.read_bytes()
    report.append(f"== {path.name}: {len(data):,} bytes")
    name = data[:16].split(b"\0")[0].decode("ascii", "replace")
    f1, f2 = struct.unpack_from("<II", data, 16)
    report.append(f"   header: name {name!r}, dword@16 {f1:,} ({f1:#x}), dword@20 {f2:,} ({f2:#x})")

    # The bytes after the stream: where the nonzero ones are.
    tail_start = 20 + f1
    tail = data[tail_start:]
    nz = [i for i, b in enumerate(tail) if b]
    report.append(f"   after {tail_start:,}: {len(tail):,} bytes, {len(nz):,} nonzero")
    if nz:
        lo = max(0, nz[0] - 16)
        hi = min(len(tail), nz[-1] + 17)
        report.append(f"   nonzero bytes from {tail_start + nz[0]:,} to {tail_start + nz[-1]:,}:")
        for k in range(lo - lo % 16, hi, 16):
            row = tail[k:k + 16]
            text = "".join(chr(b) if 32 <= b < 127 else "." for b in row)
            report.append(f"   {tail_start + k:>11,}  {row.hex(' '):47}  {text}")

    # Which way of reading the stream gives 31 TGA files? Each variant
    # stops at the first file without a valid header.
    t = time.time()
    best = None
    for reset_window, new_flags in [(True, True), (True, False), (False, True)]:
        label = (f"window {'reset' if reset_window else 'kept'}, "
                 f"flags {'new' if new_flags else 'kept'}")
        files, end = decode_files(data, f2, reset_window, new_flags, report, label)
        if best is None or len(files) > len(best[0]):
            best = (files, end, label)
    report.append(f"   ({time.time() - t:.0f} s)")

    files, end, label = best
    report.append(f"   files with [{label}]:")
    for n, (start_in, out, h) in enumerate(files):
        sig = out.find(b"TRUEVISION-XFILE")
        report.append(f"   {n:3}: input {start_in:>11,}  {describe_tga(h)}"
                      + (f"; footer signature at +{sig:,}" if sig >= 0 else ""))
        if h[6]:
            report.append(f"        id: {bytes(out[18:18 + h[6]])!r}")
    report.append(f"   stream ends at {end:,}; dword@16 + 20 = {tail_start:,}")


def ima_header_ok(b, channels):
    return all(len(b) >= 4 * (c + 1) and b[4 * c + 2] <= 88 and b[4 * c + 3] == 0
               for c in range(channels))


def wav_fmt(b):
    if b[:4] != b"RIFF" or b[8:12] != b"WAVE":
        return None
    pos = 12
    while pos + 8 <= len(b):
        cid, size = b[pos:pos + 4], struct.unpack_from("<I", b, pos + 4)[0]
        if cid == b"fmt ":
            tag, ch, rate, _, align, bits = struct.unpack_from("<HHIIHH", b, pos + 8)
            return f"WAV tag {tag:#x}, {ch} ch, {rate} Hz, {bits}-bit, block {align}"
        pos += 8 + size + (size & 1)
    return "WAV without fmt in its first bytes"


def probe_snd_entries(f, entries, report, file_size):
    flags = {}
    for n, (off, size, third, flag) in enumerate(entries):
        flags[flag] = flags.get(flag, 0) + 1
        f.seek(off)
        head = f.read(64)
        what = wav_fmt(head)
        if what is None:
            ch = 2 if flag == 3 else 1
            what = (f"IMA header ok ({ch} ch)" if ima_header_ok(head, ch)
                    else "unknown") + f": {head[:12].hex(' ')}"
        inside = off + size <= file_size
        report.append(f"   {n:5}: offset {off:>11,} size {size:>9,} third {third:>10,} "
                      f"({third:#010x}) flag {flag}{'' if inside else ' PAST END'}  {what}")
    report.append(f"   entries by flag: {dict(sorted(flags.items()))}")


def probe_snd(path, report):
    size = path.stat().st_size
    report.append(f"== {path.name}: {size:,} bytes")
    with open(path, "rb") as f:
        first = f.read(0x80)
        if path.name.lower() == "music.snd":
            pairs = [struct.unpack_from("<II", first, 8 * k) for k in range(16)]
            report.append("   header (count, offset): " +
                          ", ".join(f"({c}, {o:,})" for c, o in pairs))
            for t, (c, o) in enumerate(pairs):
                if not 0 < o < size or c > 1000:
                    report.append(f"   piece {t}: not a (count, offset) pair")
                    continue
                f.seek(o)
                entries = [struct.unpack("<4I", f.read(16)) for _ in range(c)]
                report.append(f"   piece {t}: {c} entries at {o:,}")
                probe_snd_entries(f, entries, report, size)
            end = max(o + 16 * c for c, o in pairs if 0 < o < size)
            f.seek(end)
            rest = f.read()
            report.append(f"   after the last list ({end:,}): {len(rest):,} bytes: "
                          f"{rest[:64].hex(' ')}")
        else:
            dir_off = struct.unpack_from("<I", first, 0)[0]
            count = (size - dir_off) // 16
            report.append(f"   directory at {dir_off:,}: {count} entries, "
                          f"{(size - dir_off) % 16} bytes left over")
            f.seek(dir_off)
            raw = f.read()
            entries = [struct.unpack_from("<4I", raw, 16 * k) for k in range(count)]
            probe_snd_entries(f, entries, report, size)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("install", help="the game's install folder")
    parser.add_argument("--cd", help="the CD (or a copy of it), for music.snd")
    parser.add_argument("-o", "--out", default="botf-probe.txt", help="report file")
    parser.add_argument("--only", nargs="+", metavar="FILE",
                        help="probe only these files, e.g. --only alt.res")
    args = parser.parse_args()
    report = []

    def first(name):
        return find(args.install, name) or find(args.cd, name)

    for name, probe in [("alt.res", probe_alt), ("sfx.snd", probe_snd),
                        ("english.snd", probe_snd), ("music.snd", probe_snd)]:
        if args.only and name not in [n.lower() for n in args.only]:
            continue
        path = first(name)
        if path is None:
            report.append(f"== {name}: not found")
            continue
        print(f"probing {path} ...", flush=True)
        try:
            probe(path, report)
        except Exception as e:  # keep going: a partial report still helps
            report.append(f"   failed: {type(e).__name__}: {e}")

    res = first("stbof.res")
    if res is not None and (not args.only or "music.tbl" in [n.lower() for n in args.only]):
        try:
            with zipfile.ZipFile(res) as z:
                name = next(n for n in z.namelist() if n.lower() == "music.tbl")
                data = z.read(name)
                printable = sum(1 for b in data if 32 <= b < 127 or b in (9, 10, 13))
                if printable > 0.95 * len(data):
                    report.append(f"== stbof.res/{name}: first lines")
                    report.extend("   " + line
                                  for line in data.decode("latin-1").splitlines()[:60])
                else:
                    report.append(f"== stbof.res/{name}: {len(data):,} bytes, binary; first 256:")
                    report.extend(f"   {data[k:k + 16].hex(' ')}" for k in range(0, 256, 16))
        except (StopIteration, zipfile.BadZipFile) as e:
            report.append(f"== stbof.res music.tbl: not read ({e})")

    Path(args.out).write_text("\n".join(report) + "\n", encoding="utf-8")
    print(f"Wrote {args.out}")


if __name__ == "__main__":
    main()
