# Star Trek: Birth of the Federation: data tables

Status: **record layouts known for races, ships, technologies, research
fields and buildings**; their names, pictures, models and descriptions read.
Most numeric fields still to work out. The other tables not started.
Reader: `src/engines/botf/data_table.cpp` (one `TableLayout` per table);
the viewer's Data page shows every record with its known fields and the
bytes not worked out yet.

Source: all 42 data tables of the English `stbof.res` (`.sst`, `.bst`,
`.rst`, `.tec`, `.pst`, `.est`, `.bin`, `.wtf`), whole
(`scripts/botf_res_sample.py --all-whole`), October 2026. Record sizes were
found from where the names repeat; each description link was found as the
one field whose value is, in every record, the start of a string in the
companion file.

All values little-endian. Text fields are zero-terminated inside a fixed
size; the bytes after the zero are leftovers from the tool's memory and
differ between records. Many records also hold values that are the same
in every record and look like memory addresses (`0x0040....`, `0x005A....`,
`0xBFF7....`): left over too, like the `.tdb` and `sfx.snd` ones.

## Descriptions

`racedesc.rst` (35 strings), `shipdesc.sst` (125), `techdesc.tec` (77) and
`edifdesc.bst` (318) are nothing but zero-terminated strings, one per
record of their table, in record order. A record's description field is the
byte offset of its string.

## `race.rst`: races

A `u16` count (35), then 108-byte records: the 5 major races (Cardassians,
Federation, Ferengi, Klingons, Romulans, in that order) and 30 minor races.

```
0    char[40]  name                  "Cardassians"
40   char[16]  picture               "cardn_02.tga"
56   u32       6 in every race
60   u32       description           offset in racedesc.rst
64   u16, u16  ?                     e.g. 1069, 80
68   f32       ?                     7.0 Cardassians, 10.0 Federation, 1.0, 0.7 ... for the minors
72   u8[5]     attitude to the 5 major races, 0-100; each major race has 100
               for itself (Cardassians 100 65 70 80 75; Federation 65 100 80 75 70)
77   u8        106 in every race
78   ...       not worked out; ends with a f32 between 0.1 and 2.0
```

## `shiplist.sst`: ship classes

No header: 125 records of 288 bytes (the file's size / 288).

```
0    char[40]  name                  "Strike Cruiser"
40   char[20]  class                 "Keldon"
84   u32       description           offset in shipdesc.sst
88   char[4]   model                 "ch3": the ch3_a/_b/_c .hob models and their .gif textures
100  ...       not worked out
```

From comparing records: 100 differs in every ship; 108-172 hold a few
dozen small integers that vary by ship and are 0 for colony ships and
transports (weapon, hull, cost and speed values, by the look of them);
212-271 are eight `f64` values (100.0, 180.0 ...), zero for some ships;
several other fields are constant.

## `techtree.tec`: technologies

No header: 77 records of 72 bytes, 7 research fields x 11 levels, in field
order.

```
0    char[30]  name                  "Basic Biotech"
40   u16       number                0-76
42   char[6]   the field's short name   "bio", "comp", "cnst", "engy", "prop", "weap", ...
55   u8        field                 0-6, as in tecfield.tec
56   u8        level                 0-10
60   u32       cost                  0, 0, 200, 1100, 4000, 9000 ... 81700 in biotech: rises with the level
68   u32       description           offset in techdesc.tec
```

## `tecfield.tec`: research fields

A `u16` count (7), then 41-byte records: name (`char[40]`: biotech,
computer, construction, energy, propulsion, weapons, sociology) and its
number (`u8`, 0-6).

## `edifice.bst`: buildings

```
0      u32  count (318)
4      u32  where the records end (56,708 = 740 + 318 x 176)
8      ...  lists of record offsets (into this file), probably groups of buildings
740    318 records of 176 bytes
56708  1,272 more bytes of record offsets
```

A record:

```
0    char[40]  name                  "Primitive Farm"
40   char[48]  plural                "Primitive Farms"
88   char[8]   picture               "T0fm0"
172  u32       description           offset in edifdesc.bst
```

Each building type appears once per race that can build it (the first five
records are all "Primitive Farm").

## Not worked out yet

| Files | Size | Notes |
|---|---|---|
| `shiptech.sst`, `shiprace.sst` | 2,000, 140 | ship requirements and races, no text |
| `edifbnft.bst` | 849 | benefit names ("Food", "% food", "Energy" ...) |
| `racetech.tec` | 36,400 | no text |
| `planet.pst` | 85,152 | planets: animation names (`aros.ani`) and file paths |
| `environ.est` | 780 | planet environments (`arctic`, `barren` ...) |
| 18 `.bin` | 112 to 62,644 | names (`shipname.bin`, `starname.bin`), rules (`morale.bin`, `ppgrowth.bin`, `planboni.bin`, `combat.bin` ...), AI (`aibldreq.bin`, `bldset1-5.bin`) |
| 7 `.wtf` | 578 to 2,913 | AI weights, colonisation values per race |
