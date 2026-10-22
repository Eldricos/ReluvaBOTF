# Star Trek: Birth of the Federation: data tables

Status: **record layouts known for races, ships, technologies, research
fields, buildings, planets, environments, star and ship names, ships'
requirements and races, and planet values**; their names, pictures, models and descriptions read;
ships' combat values and costs, and buildings' costs, maintenance and
output, matched against the game's screens; the rest of the ship and
building records from the modding community's editor, checked against the
data (see "Other sources"). Some numeric fields still to work out.
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
40   char[44]  class                 "Keldon"
84   u32       description           offset in shipdesc.sst
88   char[14]  model                 "ch3": the ch3_a/_b/_c .hob models and their .gif textures
102  u16       number                the record's own number, in every record
104  u16       role                  0 scout, 1 destroyer, 2 cruiser, 3 strike cruiser, 4 command
                                     ship, 5 colony ship, 6 outpost, 7 starbase, 8 monster, 9 troop transport
106  u16       name group            in shipname.bin
108  u32       beam accuracy         (125, 115, 140 ...; 0 without beams)
112  u32       beam weapons          how many
116  u32       beam damage multiplier   1 in every ship
120  u32       beam damage           each
124  u32       beam shield penetration  0 in every ship
128  u32       torpedo accuracy
132  u32       torpedoes             how many
136  u32       torpedo damage multiplier   1 in every ship
140  u32       torpedo damage        each
144  u32       shield level          0 in every ship
148  u32       shields
152  u32       shield recharge       rises with the shields
156  u32       offensive strength    for the AI, worked out from the weapons
160  u32       defensive strength    for the AI, worked out from shields, hull and defense
164  u32       ?                     0 in every ship
168  u32       hit points
172  u32       defense               rises through each line of classes (35, 75 ...)
176  u32       stealth               2 for most later classes; 6 for the Romulan warbirds (cloaking)
180  u32 x5    extra beam weapon     how many, accuracy, multiplier, damage, penetration: used
                                     only by the Galaxy II (1, 150, 8, 80, 12)
200  u32       ?                     1 for the Galaxy II only
204  8 bytes   ?                     0
212  f64       agility               100.0; more for the Federation's escorts (120-200); 0 for
                                     outposts and starbases
220  f64       firing arc            degrees: 90 to 360
228  f64       beam range
236  f64       torpedo minimum range
244  f64       torpedo range
252  f64       extra beam range
260  f64       range                 0 short, 1 medium, 2 long
268  f64       speed                 1, 2, 3
276  u16       scan range            0; 1 for scouts and outposts, 2 for starbases
278  u16       production            colony ships 1 or 3 (terraforming), troop transports 400 or
                                     1600 (ground combat), 0 for the rest
280  u16       race                  in race.rst
282  u8, u8    beam and torpedo name prefixes; used only by a patched executable
284  u16       build cost            industry
286  u16       maintenance cost
```

Where these came from: build cost, maintenance, beams, torpedoes, shields,
hit points, range and speed were matched against the game's screens (below).
The rest are from the Ultimate Editor's reader (see "Other sources" at the
end) and checked against the data, as follows:
- the number is the record's position in all 125 records;
- the race agrees with `shiprace.sst`'s blocks in all 125 (records 0-19 race
  0, 20-44 race 1 ..., the Talarians' ship, 110, race 28);
- every name group exists in `shipname.bin`, and the names fit the class:
  the Nebula class's group starts "USS Nebula", the Miranda's "USS Miranda",
  the Oberth's "USS Oberth", the Ambassador's "USS Ambassador";
- the roles fit the names (every scout 0, colony ship 5, outpost 6, starbase
  7, troop transport 9, monster 8);
- production is non-zero only for colony ships and troop transports;
- stealth is highest for the Romulan warbirds, which cloak.

Accuracy, multipliers, penetration, defense, agility, firing arc and the
weapon ranges are combat values: the screens recorded don't show them, so
their meanings are UE's until ship combat is observed.

The ship list's stats for five Federation classes, and their records:

| Class (record) | Build cost | Hit points / shields | Beams | Torpedoes | Range / speed |
|---|---|---|---|---|---|
| Edward colony ship (22) | 1210 | 55 / 70 | none | none | Medium / 1 |
| Miranda destroyer (30) | 1410 | 62 / 220 | 4 x 15 | 3 x 28 | Medium / 1 |
| Oberth scout (35) | 490 | 19 / 110 | 2 x 15 | 2 x 28 | Long / 1 |
| London transport (39) | 1030 | 43 / 110 | none | none | Medium / 1 |
| Ambassador cruiser (43) | 3100 | 153 / 410 | 7 x 25 | 5 x 44 | Medium / 1 |

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
8      ...  counters per research field, a map of buildings by research field and
            level (record offsets into this file), then per major race a 384-bit
            mask of buildings (6 u64): UE calls it the ignore list
740    318 records of 176 bytes
56708  318 u32: each building's record offset, by building number
```

A record:

```
0    char[40]  name                  "Primitive Farm"
40   char[40]  plural                "Primitive Farms"
80   u16       number                each building's own number, 0-317 (not the record's position)
82   u16       upgrade of            the number of the building it replaces; 32767 for none
84   4 bytes   ?
88   char[8]   picture               "T0fm0"
96   5 bytes   ?
101  u8        research field needed    as in tecfield.tec
102  u8        technology level needed
103  u8        races that can't build it   a bit per major race (bit 1: the Federation)
104  u64       resident races needed    a bit per race; 0x7ffffffff (all 35) for none
112  u8        system status needed     0 home system, 1 native member, 2 non-native member,
                                        3 subjugated, 4 affiliated, 5 independent minor,
                                        6 conquered home; 9 for none
113  u8        planet or star needed in the system   8 for none
114  u8        output type              which of edifbnft.bst's 45 wordings the output takes:
                                        0 " Food", 2 " Energy", 13 " Industry", 17 " Anti-Ship
                                        Defenses", 36 " Morale", 37 "% ground defense" ...
115  u8        build limit              0 none, 1 one per system, 2 one per empire
116  u8        group                    the main buildings' line per race (farms, factories ...);
                                        34 for the rest
117  u8        category                 0 research, 1 industry, 2 food, 3 energy, 4 defense
118  u16       ?                        0
120  u16       build cost               industry
122  u16       ?                        0
124  u16       energy maintenance
126  u16       ?                        0
128  u16       population maintenance
130  i16       morale                   1 for the academies, -1 for the Labor Camp
132  u16       ?                        0
134  i16[6]    output                   per race: the 5 major races in race.rst's order, then one more
146  i16[6]    unrest bonus             per race, the same way: Private Farms' 1 six times, the Labor
                                        Camp's -1 for the Cardassians
158  14 bytes  ?                        0
172  u32       description              offset in edifdesc.bst
```

The records are in the order the tool wrote them, not by number. The table of
offsets after the records gives each building's record by number: entry *n*
is the file offset of the record whose number is *n*, for all 318.

Build cost, maintenance and output were matched against the game's screens
(below). The rest are from the Ultimate Editor's reader, checked against the
data: the numbers are 0-317 with none missing, 206 of the 208 upgrades point
at the previous level of the same line (Type 4 Automated Farm, 147, upgrades
146; the other two are scanners that skip levels),
and where only one race may build a building and the outputs differ, the
biggest output is that race's in all 10 cases (Starfleet Academy 0 50 0 0 0 0,
Federation only; Tactical College 0 0 0 45 0 0, Klingons only; Naval Academy
0 0 0 0 35 0, Romulans only). The output type is the index of the output's
wording in `edifbnft.bst`, which the screens confirm: the Orbital Battery's
17 is " Anti-Ship Defenses" (+80 on screen), the Bunker Network's 37 "% ground
defense" (+50%), Martial Law's 36 " Morale" (+7), Starfleet Academy's 16
" Ship experience" (+50).

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
offsets of the strings. A building's output type (`edifice.bst` offset 114)
is the number of its wording here.

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

## Other sources

Fields marked as from "UE" come from the Ultimate Editor, the BotF modding
community's editor (Java, GPL-2.0, by DCER and others):
<https://gitlab.com/stbotf/ultimate-editor>, read at commit `7ee45c4b`
(July 2023), October 2026. Only the layouts were taken from it (offsets,
sizes, what its authors call each field); no code was copied. Its knowledge
comes from two decades of the community's work, including reading the game's
executable, discussed on the Armada Fleet Command forums
(<https://www.armadafleetcommand.com/onscreen/botf/>). Every field taken from
it was checked against the data before being listed here, and the check is
given with the field; a meaning that the data can't check (most combat
values) says so.
