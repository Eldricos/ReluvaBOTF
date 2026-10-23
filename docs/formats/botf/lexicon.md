# Star Trek: Birth of the Federation: `lexicon.dic`

Status: **layout known** (from the modding community's editor); reader
`src/engines/botf/lexicon.cpp`. Not yet checked against the file itself.

The words and short phrases the interface shows: option values ("Early",
"Many", "Normal"), race, star and planet type names, labels. 81,577 bytes in
the English `stbof.res`; it differs in the German one.

```
0     u32       where the table is
4     the strings, zero-terminated, Windows-1252
table u32       not known
      u16       how many entries
      per entry: u32 the string's offset, u32 its length (with the zero)
```

Which entry a screen shows isn't in the file: the executable picks it. The
numbers the game setup screen uses are from the Ultimate Editor
(`ue/edit/res/stbof/files/dic/idx/LexDataIdx.java`, GPL-2.0, read for the
numbers only; see [tables.md](tables.md#other-sources)), where each is
marked "confirmed by value change": its authors changed the entry and saw
the screen change. In `lexicon.h` (`lex::`):

| Setting | Entries |
|---|---|
| Civilization level | 88 Beginning, 252 Early, 234 Developed, 312 Expanded, 4 Advanced |
| Minor races | 872 None, 358 Few, 1161 Some, 622 Many |
| Difficulty | 1150 Simple, 256 Easy, 873 Normal, 433 Hard, 482 Impossible |
| Timers | 872 None |
| Random events | 880 Off, 882 On |
| Tactical combat | 621 Manual, 52 Automatic |
| Victory conditions | 248 Domination, 1407 Vendetta, 1253 Team Play |
| Galaxy shape | 555 Irregular, 264 Elliptical, 1056 Ring, 1172 Spiral |
| Galaxy size | 1158 Small, 627 Medium, 596 Large |

The recording shows the civilization levels as "2 EARLY": whether the
number is part of the entry or added by the game will show once the file's
read.
