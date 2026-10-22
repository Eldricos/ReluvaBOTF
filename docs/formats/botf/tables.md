# Star Trek: Birth of the Federation: data tables

Status: **record layouts known for races, ships, technologies, research
fields, buildings, planets, environments, star and ship names, ships'
requirements and races, and planet values**; their names, pictures, models and descriptions read;
ships' combat values and costs, and buildings' costs, maintenance and
output, matched against the game's screens. Many numeric fields still to
work out. The other tables not started.
Reader: `src/engines/botf/data_table.cpp` (one `TableLayout` per table);
the viewer's Data page shows every record with its known fields and the
bytes not worked out yet.

Source: all 42 data tables of the English `stbof.res` (`.sst`, `.bst`,
`.rst`, `.tec`, `.pst`, `.est`, `.bin`, `.wtf`), whole
(`scripts/botf_res_sample.py --all-whole`), October 2026. Record sizes were
found from where the names repeat; each description link was found as the
one field whose value is, in every record, the start of a string in the
companion file.

Numbers were named by matching the stats the game shows against the
records: a recording of the original game ("Let's Play Star Trek Birth of
the Federation, UFP, p1", 49 minutes, playing the Federation; screens
found by their layout and read from stills), October 2026. A field is
named only when every screen shown agrees with it.

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
108  u32       ?                     beam weapons' (125, 115, 140 ...; 0 without beams): accuracy?
112  u32       beam weapons          how many
116  u32       1 in every ship
120  u32       beam damage           each
128  u32       ?                     torpedoes' (114, 112, 121 ...; 0 without torpedoes)
132  u32       torpedoes             how many
136  u32       1 in every ship
140  u32       torpedo damage        each
148  u32       shields
168  u32       hit points
260  f64       range                 0 short, 1 medium, 2 long
268  f64       speed                 1, 2, 3
278  u16       ?                     400 for the London Class transport, 1 for the colony ship, 0 for warships: what it carries?
282  u16       10 in the ships seen
284  u16       build cost            industry
286  u16       maintenance cost
```

The ship list's stats for five Federation classes, and their records:

| Class (record) | Build cost | Hit points / shields | Beams | Torpedoes | Range / speed |
|---|---|---|---|---|---|
| Edward colony ship (22) | 1210 | 55 / 70 | none | none | Medium / 1 |
| Miranda destroyer (30) | 1410 | 62 / 220 | 4 x 15 | 3 x 28 | Medium / 1 |
| Oberth scout (35) | 490 | 19 / 110 | 2 x 15 | 2 x 28 | Long / 1 |
| London transport (39) | 1030 | 43 / 110 | none | none | Medium / 1 |
| Ambassador cruiser (43) | 3100 | 153 / 410 | 7 x 25 | 5 x 44 | Medium / 1 |

Not yet named: 100 differs in every ship; 152, 156, 160 and 172 vary by
ship (152 rises with the shields: 10, 26, 13, 13, 45); 212-259 are six more
`f64` values (100.0, 270.0 or 360.0, 300.0-700.0 ...), zero for unarmed
ships; several other fields are constant.

Range and speed (the last two `f64`) and the maintenance cost were
confirmed on the whole Let's Play series (38 parts): every ship class whose
range or speed the screens show, 19 of them, agrees (Galaxy and Nebula
classes short, Oberth long, Destroyer II speed 2, Heavy Destroyer speed 3),
and the maintenance costs read from the stills (23, 73, 115, 134) agree. An
earlier guess, range at 276, was wrong: the Galaxy class shows "Short" with
the same 0 there as medium-range ships.

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
120  u16       build cost            industry
124  u16       energy maintenance
128  u16       population maintenance
134  u16[6]    output                per race: the 5 major races in race.rst's order, then one more
146  u16[6]    second bonus          per race, the same way: Private Farms' +1 morale (1 six times)
172  u32       description           offset in edifdesc.bst
```

Each building type appears once per race that can build it (the first five
records are all "Primitive Farm").

The screens shown and the records (Federation buildings):

| Building (record) | On screen | Build cost | Energy / population maintenance | Output |
|---|---|---|---|---|
| Type 2 Automated Farm (14) | +30 food, cost 165, population maintenance 10 | 165 | 0 / 10 | 30 |
| Bunker Network (91) | +50% ground defense, cost 300, energy maintenance 10 | 300 | 10 / 0 | 50 |
| Orbital Battery (142) | +80 anti-ship defenses, cost 500, energy maintenance 50 | 500 | 50 / 0 | 80 |
| Martial Law (187) | +7 morale | 0 | 0 / 0 | 1 7 1 1 1 1 |
| Mythology Library (207) | +300 research, cost 1500, energy maintenance 50 | 1500 | 50 / 0 | 300 |
| Private Farms (236) | +20 food, +1 morale, cost 1500, energy maintenance 80 | 1500 | 80 / 0 | 20, second bonus 1 |

The output is per race, not per difficulty level: the Federation (race 1)
is shown the second value. Martial Law's are 1 7 1 1 1 1 and it shows +7
morale; Starfleet Academy's are 0 50 0 0 0 0 and it shows +50 ship
experience. Every other building seen has the same value six times.

Over the whole series (38 parts), 129 buildings were matched by name and
numbers: 100 build costs, 28 energy maintenance, 61 population maintenance
and 82 outputs agree. Every disagreement left is a misread digit in the
OCR (5060 read 5080, 41 read 4, 50 read 90, 10 read 0 or 19 ...), checked
against the stills where it mattered.

Upgrade screens agree with the output and population maintenance of the
Type 3 Plasma Reactor (26 to 31 energy), Mass Replicator (15 to 20
industry), Databank (16 to 26 intelligence) and University (17 to 22
research): output 31, 20, 26, 22 and population maintenance 10 each. Their
upgrade prices aren't the build cost at 120.

## `shiptech.sst`: what ships need

No header: 125 records of 16 bytes, one per ship class in `shiplist.sst`'s order.

```
0    u16       race                  0-4 the major races; 36 for the Tarellian vessel (a monster)
4    u16       ship                  its number in shiplist.sst
6    u16       upgrades from         the ship it replaces (Colony Ship II: 2, Colony Ship); 65535 for none
8    u8[6]     technology levels     needed in six research fields (Strike Cruiser 2 3 4 3 2 4; colony ships 1 1 1 1 1 1)
```

## `shiprace.sst`: each race's ships

35 `u32`, one per race in `race.rst`'s order: the number of the race's first
ship in `shiplist.sst` (Cardassians 0, Federation 20, Ferengi 45, Klingons
65, Romulans 85, then 105 to 114 for the minor races that have one), or
4294967295 for a race without ships.

## `shipname.bin`: ship names

```
0     u32[62]  how many names in each group
248   u32      252: where the names start
252   1,418 records of 44 bytes:
        u16       group               0-61
        u16       65535 (27 in one record)
        char[40]  name                "USS Galaxy"
```

Groups of 30 names (6 or 7 for some, 1 for a transport, outpost or
starbase), by race prefix: 1-11 Federation (`USS`), 12-21 Klingon (`IKC`),
22-31 Romulan (`RNS`), 32-39 Ferengi (`FMS`), 40-49 Cardassian (`CDS`), 50
outpost, 51 starbase, 52-61 others. Group 0 is "UPN Invalid". Which group a
ship class takes its names from isn't known yet.

## `starname.bin`: star names

A `u32` count (297), then 24-byte records: the name (`char[20]`, up to 12
letters: "Exo", "Ultima Thule") and its length + 1 (`u32` at 20).

## `environ.est`: planet types and atmospheres

```
0     u16       count (13)
2     f32[13]   how common each is: 0.19 0.22 0.14 0.10 0.05 0.07 0.04 0.19 for the 8 planet types, 0 for the atmospheres; they add up to 1
130   13 records of 50 bytes: name (char[40]), number (u16 at 40), then values not worked out
```

0 arctic, 1 barren, 2 desert, 3 gas giant, 4 jungle, 5 oceanic, 6 terran, 7
volcanic; 8 methane, 9 none, 10 oxygen rich, 11 sulfuric, 12 thin oxygen.

## `planet.pst`: planets

A `u16` count (1,117) and leftovers to 260, then 76-byte records:

```
0    char[40]  name                  "1", "2" ... or "Mars"
40   char[12]  animation             "aros.ani": its letters spell type, atmosphere and size
54   u16       number                different in every record
56   u16       size                  0 small, 1 medium, 2 large (the animations' s, m, l)
62   u16[3]    ?                     each 0, 310 or 620
72   u8        type                  environ.est's 0-7
73   u8        atmosphere            environ.est's 8-12
```

The animation names agree with the type, atmosphere and size in every
record but those of the Sol system's own planets (Mercury, Earth, Mars,
Jupiter, Saturn, Uranus, Neptune, Pluto have their own animations):
`a`/`b`/`d`/`g`/`j`/`o`/`t`/`v` for the type, `r` (oxygen rich), `t` (thin
oxygen) ... for the atmosphere, then `s`, `m`, `l`.

## `planspac.bin` and `planboni.bin`: values per planet type and size

Each 21 records of ten `u32`: a key, planet type + 256 x size (the 7 types
other than gas giants, 3 sizes each), and 9 values.

- `planspac.bin` ("planet space"?): 25 to 140, growing with the size and
  highest for terran and jungle worlds (terran large: 120 125 130 130 130
  130 135 135 140): maximum population, presumably, for 9 conditions not
  known yet.
- `planboni.bin`: 0 for arctic and barren worlds; 0, 5, 10, 15, 20 in a
  pattern repeating every 3 values for desert and volcanic worlds; 0-10,
  changing every 3 values, for jungle, oceanic and terran ones. Bonuses,
  presumably (energy and food?).

## `edifbnft.bst`: building benefits

45 wordings for building output (" Food", "% food", " Energy", "% total
credits", " Ship experience", " Anti-Ship Defenses" ...): a `u32` (669: where
the offset table starts), the zero-terminated strings, then 45 `u32`
offsets of the strings.

## Not worked out yet

| Files | Size | What's known |
|---|---|---|
| `racetech.tec` | 36,400 | 325 offsets (1,300 bytes), then 325 records of 108 bytes |
| `morale.bin` | 460 | 115 `i32`: five 50s, then values in groups of five (one per major race?), many negative (-10, -7 ...) |
| `ppgrowth.bin` | 120 | 10 records of 12 bytes: two `u16` (10, 50 ...), a `u32` halving from 1,000,000 to 1,250, and a flag |
| `intel.bin` | 480 | 120 `i32`, in groups of 24 (one per major race?) |
| `meplanet.bin`, `meprvnum.bin`, `habit.bin` | 2,640, 120, 112 | `meplanet.bin` holds names ("Cardassia" ...) |
| `aibldreq.bin`, `bldset1-5.bin` | 1,848, 280-792 | AI building rules |
| `combat.bin`, `converse.bin` | 62,500, 16,714 | |
| 7 `.wtf` | 578 to 2,913 | AI weights, colonisation values per race |
