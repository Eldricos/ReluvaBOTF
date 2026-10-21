#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Reluva contributors
"""Check the layouts worked out from botf_sample.py's samples against whole files.

Reads (never writes) Star Trek: Birth of the Federation's install folder,
and optionally the CD, and writes a text report about:

- alt.res: decompresses the whole LZSS stream (docs/formats/botf/alt-res.md)
  and lists what it holds: each TGA header found, and anything else
- sfx.snd, english.snd, music.snd: every directory entry, with the first
  bytes of each sound (docs/formats/botf/snd.md)
- music.tbl from stbof.res: its first lines

The report holds no images or sounds, only headers, sizes and offsets.
Only standard-library Python 3.8+ is needed:

    python3 botf_probe.py "C:\\Program Files\\MicroProse\\Trek" --cd D:\\ -o botf-probe.txt

Decompressing alt.res takes a minute or two.
"""

import argparse
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


def lzss_decompress(src, start, end):
    """The LZSS variant alt.res uses: 4 KB window starting at 0xfee, zero-filled.

    A flag byte gives the kind of the next 8 items, low bit first: 1 = a literal
    byte, 0 = a two-byte reference ab cd = length a + 2, window position bcd.
    """
    window = bytearray(4096)
    r = 0xFEE
    out = bytearray()
    i = start
    flags = 0
    while i < end:
        flags >>= 1
        if not flags & 0x100:
            flags = src[i] | 0xFF00
            i += 1
            if i >= end:
                break
        if flags & 1:
            c = src[i]
            i += 1
            out.append(c)
            window[r] = c
            r = (r + 1) & 0xFFF
        else:
            if i + 1 >= end:
                break
            a, b = src[i], src[i + 1]
            i += 2
            pos = ((a & 0x0F) << 8) | b
            for k in range(((a >> 4) & 0x0F) + 2):
                c = window[(pos + k) & 0xFFF]
                out.append(c)
                window[r] = c
                r = (r + 1) & 0xFFF
    return out, i


def tga_header(data, off):
    """(width, height, bits, descriptor, size) if a plain TGA header is at `off`."""
    if off + 18 > len(data):
        return None
    id_len, cmap_type, img_type = data[off], data[off + 1], data[off + 2]
    if cmap_type != 0 or img_type not in (2, 10) or id_len > 64:
        return None
    w, h, bits, desc = struct.unpack_from("<HHBB", data, off + 12)
    if not (0 < w <= 4096 and 0 < h <= 4096 and bits in (15, 16, 24, 32)):
        return None
    size = 18 + id_len + w * h * ((bits + 7) // 8) if img_type == 2 else None
    return w, h, bits, desc, size, img_type, id_len


def probe_alt(path, report):
    data = path.read_bytes()
    report.append(f"== {path.name}: {len(data):,} bytes")
    name = data[:16].split(b"\0")[0].decode("ascii", "replace")
    f1, f2 = struct.unpack_from("<II", data, 16)
    report.append(f"   header: name {name!r}, dword@16 {f1:,} ({f1:#x}), dword@20 {f2:,} ({f2:#x})")
    report.append(f"   bytes after dword@16's end ({20 + f1:,}): "
                  f"{len(data) - 20 - f1:,}, nonzero among them: "
                  f"{sum(1 for b in data[20 + f1:] if b):,}")
    t = time.time()
    out, used = lzss_decompress(data, 24, len(data))
    report.append(f"   decompressed from offset 24: {len(out):,} bytes, input used up to "
                  f"{used:,}, {time.time() - t:.0f} s")
    # Walk the output: TGA after TGA, reporting anything that isn't one.
    off = 0
    count = 0
    while off < len(out):
        h = tga_header(out, off)
        if h and h[4]:
            w, hgt, bits, desc, size, img_type, id_len = h
            report.append(f"   {off:>11,}: TGA {w}x{hgt} {bits}-bit, descriptor {desc:#04x}, "
                          f"id length {id_len}, {size:,} bytes")
            count += 1
            off += size
            continue
        # Not a TGA: show it, then look for the next TGA header.
        report.append(f"   {off:>11,}: not a TGA: {out[off:off + 48].hex(' ')}")
        nxt = off + 1
        while nxt < len(out) and not (out[nxt + 2:nxt + 3] == b"\x02" and tga_header(out, nxt)):
            nxt += 1
        if nxt >= len(out):
            tail = out[off:]
            report.append(f"   {off:>11,}: {len(tail):,} more bytes to the end, "
                          f"{sum(1 for b in tail if b):,} nonzero; last 32: {tail[-32:].hex(' ')}")
            break
        report.append(f"   skipped {nxt - off:,} bytes")
        off = nxt
    report.append(f"   TGA images: {count}")


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
    args = parser.parse_args()
    report = []

    def first(name):
        return find(args.install, name) or find(args.cd, name)

    for name, probe in [("alt.res", probe_alt), ("sfx.snd", probe_snd),
                        ("english.snd", probe_snd), ("music.snd", probe_snd)]:
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
    if res is not None:
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
