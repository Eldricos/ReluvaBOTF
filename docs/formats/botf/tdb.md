# Star Trek: Birth of the Federation: `.tdb` text tables

Status: **readable**: every string and its key. Open: what the keys mean.
Reader: `src/engines/botf/text_table.cpp`; the viewer's Text page shows
them.

Source: the first 2 KB of all 22 tables in the English `stbof.res`, and
three whole tables, `treaty.tdb`, `minor6.tdb` and `cdiplomt.tdb`
(`scripts/botf_res_sample.py --ext tdb`), October 2026.

| Tables | Strings | Text |
|---|---|---|
| `cdiplomt`, `fdiplomt`, `hdiplomt`, `kdiplomt`, `rdiplomt` | 221 each | what each major race says in diplomacy |
| `minor1` to `minor9` | 71 each | what the minor races say |
| `treaty` | 10 | treaty descriptions ("%s1 and %2 have formed a strategic alliance...") |
| `inttext0` to `inttext4`, `intqueue` | 60 to 301 | intelligence reports |
| `teachtip` | 394 | tutorial tips |

All values little-endian.

```
offset 0          u32 count
offset 4          u32 ?          a value like 0x005A0628, different in every table: left over from the tool, like sfx.snd's
offset 8          u32 longest    the longest string's length, zero byte included
offset 12         u32 0
offset 16         count records of 16 bytes:
                    u32 key
                    u32 length   of the string, zero byte included
                    u32 ?        like the header's second value, different in every record
                    u32 offset   of the string, from the end of the records
16 + 16 x count   the strings, each ending in a zero byte, packed with no gaps
```

How it was checked: in the three whole tables (10, 71 and 221 strings)
every string ends in a zero byte, the strings tile the text with no gaps or
overlaps, and the last ends at the end of the file. In the 2 KB samples,
`longest` equals the longest length in every table whose records all fit,
and the header's last value is 0 in all 22. The strings aren't stored in
record order: `treaty.tdb`'s record 4 points at offset 0.

## The strings

All plain ASCII in the English tables (Reluva reads them as Windows-1252,
which has every accented letter the fonts have, for other languages). Each
string starts with a space and ends with a newline before its zero byte;
Reluva's viewer leaves both out. Placeholders, filled in by the game:
`%1`, `%2`, `%3` and `%s1` (in the three whole tables).

## Keys

Records are sorted by key in most tables; in `intqueue`, `inttext2`,
`inttext3` and `inttext4` they aren't, and keys may repeat there. The keys
look structured but aren't understood yet:

- diplomacy, minor-race and treaty tables: the third byte is `0x41`, `0x42`,
  `0x44` or `0x48` (`A`, `B`, `D`, `H`), the low byte varies (`07`, `08`,
  `37` in `treaty.tdb`; `92`, `93`, `94` together for a proposal and its
  two answers in the minor tables). The five diplomacy tables share their
  key layout (61, 63 and 3 keys of the first three kinds in the first 2 KB).
- `teachtip.tdb`: keys under 65,536.
- the intelligence tables: high words from 648 to 1,340.

Which key the game asks for in which situation is for the running game to
show.
