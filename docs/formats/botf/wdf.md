# Star Trek: Birth of the Federation: window layouts (`.wdf`)

Status: **layout known; checked against all 377 English files**, which
all read. 377 files in `stbof.res`, one per screen,
dialog or panel (`options.wdf`, `gmesetup.wdf`, `emppick.wdf`,
`tactical.wdf` ...). Reader: `src/engines/botf/window_layout.cpp`; the
viewer's Layouts page draws each one.

Source: the Ultimate Editor's reader and window editor (Java, GPL-2.0,
<https://gitlab.com/stbotf/ultimate-editor>, commit `7ee45c4b`, files
`ue/edit/res/stbof/files/wdf/` and `ue/gui/stbof/wdf/`), read for the layout
only, October 2026; see [tables.md](tables.md#other-sources). Checked
against every `.wdf` of the English `stbof.res`
(`scripts/botf_res_sample.py --ext wdf mnu --all-whole`), October 2026: all
377 read, their widgets drawn where the game shows them (by eye, on the
viewer's Layouts page).

All values little-endian; `i32` unless said otherwise. Names of files are 13
bytes, zero-terminated, with leftovers after the zero (the editor's comments
note one, in `c_tfship.wdf`, where a name runs into the next field); "none"
means no file.

## File

```
0    i32       2 in every file
4    i32       1: the type of the first widget, always a window
8    the window
```

## Widgets

Every widget starts the same way:

```
0    i32       id
4    i32       x        from its window's corner
8    i32       y
12   i32       width
16   i32       height
20   char[13]  background picture (a .tga in stbof.res)
```

Inside a window, each widget is preceded by its type (`i32`):

| Type | Widget | After the common part |
|---|---|---|
| 1 | window | how many widgets (`i32`), how many of each type 1-15 (`i32[15]`), then the widgets, grouped by type in that order (the editor says the game crashes otherwise) |
| 2 | button | `i32` -1, `u8` kind (0, 1 or 2), `i32` repeats when held (0, 1), `i32` repeats a second, normal picture, selected picture |
| 5 | menu | four values (0, 0, then the widget's x and y; see Menus), two fonts, the menu (`char[13]`, a `.mnu`), left, top, right and bottom pictures, one more file name |
| 7 | slider | `i16` 0, maximum, how many cells, the bar's width (the widget's), `i32` 0 or the maximum again, cell off, cell on, marker off and marker on pictures, the bar's height (about the widget's), `i32` 0, `i32` 0. UE reads the last three as the maximum, cells and width again, but they aren't (an intelligence slider: 310, 31 cells, 186 wide; then 22, 0, 0) |
| 8 | scroll bar | `i32` 0 or 1, handle on and handle off pictures, minimum, maximum |
| 12 | list box | `i32` -1, two fonts, 10 column widths, how many columns (1-10) |
| 13 | picture | `i32` -1 |
| 14 | text box | text rectangle (x, y, width, height), a font |

In 93 of the 377 files more widgets follow the last one the counts cover
(in `c_ship.wdf`, a whole text box): leftovers, like those after the
names. They're not read.

Windows hold windows, so a layout is a tree. A widget's place on the screen
is its own x and y plus those of every window it's in, and the editor
draws each picture cut to its widget's rectangle.

A font (48 bytes):

```
0    u8[3]     colour: red, blue, green (in that order, as the editor reads it)
3    i32[7]    not known: all 0, or a single 1 in one of the first four
31   i32       1: use this font, 0: the game's default
35   char[13]  the font (a .fnt in stbof.res)
```

Most fonts (1,690 of 1,760) say 0, "use the game's default", with colour
0; the 70 that don't are in the options and game setup screens, the
save/load list, and each race's system, labour, tech and tactical screens.
The fonts named are the race fonts (`cardsn`, `ferngi`, `human`, `klingn`,
`romuln`) at 9, 15, 20 and 28.

## Menus

A menu widget names a `.mnu`, the items of its pop-up menu. There are
three, each used by the five races' screens:

- `system.mnu` (in `?solars.wdf`, the system screen): one item, "home".
- `fleets.mnu` (in `?tactscn.wdf`, tactical combat, twice): the task force
  kinds, 1 Escorts, 2 Artillary (sic), 3 Capital, 4 Non-Combat, 5 Bases.
- `combat.mnu` (in `?tactscn.wdf`): the combat orders, for one ship (1-12)
  and for a group (101-112): Charge, Assault, Harry, Circle, Strafe, Flyby,
  Evade, Ram, Retreat, Open hailing frequencies (8 and 11 aren't used);
  then 200 Attack, 201 Open hailing frequencies, 900 Select Ship, 901
  Select Group.

```
0    u32       1
4    u32       how many items
8    88 bytes an item:
       0   i32       its number (what the game acts on, presumably)
       4   char[13]  a picture ("none" in all of them)
       17  char[31]  the label
       48  40 bytes  the same in every item: 359, 0, three values that look like
                     addresses, 0, 0, 359, 0, 1
```

Read by `src/engines/botf/menu_file.cpp`; worked out from the three files
(UE doesn't read them), October 2026. The menus are in the language's
`stbof.res` (the German ones differ).

The menu widgets themselves (four values after the common part, then two
fonts): the four values are 0, 0 and the widget's own x and y again, not a
text rectangle as UE has it; the first font is the default (0) and the
second has its own colour, per race in the system screen and red
(`ff 00 00`) in combat: the highlight, presumably. How the game frames the
menu with the four pictures is still to be seen; the viewer shows it open,
the items in the menu's font.

## Pictures and transparency

The pictures are 16-bit TGAs. What isn't to be drawn is magenta (red and
blue full, green 0), which shows as large pink areas if drawn as it is: the
game's renderer skips a colour key (`trek.exe` imports MPR's
`MPRSetChromaKey`), set by the executable rather than stored in the files.
The viewer treats magenta as transparent (`applyColorKey`); the Images page
shows the pictures both ways (K).

## Not known yet

- What the button kinds, the fonts' seven values and the 0s and -1s mean.
- How the game finds a widget: presumably by its id, from the executable.
- Whether the colour order (red, blue, green) is right: the screens in the
  Let's Play can settle it.
