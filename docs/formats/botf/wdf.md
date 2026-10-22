# Star Trek: Birth of the Federation: window layouts (`.wdf`)

Status: **layout from the modding community's editor; not yet checked
against the game's files.** 377 files in `stbof.res`, one per screen,
dialog or panel (`options.wdf`, `gmesetup.wdf`, `emppick.wdf`,
`tactical.wdf` ...). Reader: `src/engines/botf/window_layout.cpp`; the
viewer's Layouts page draws each one.

Source: the Ultimate Editor's reader and window editor (Java, GPL-2.0,
<https://gitlab.com/stbotf/ultimate-editor>, commit `7ee45c4b`, files
`ue/edit/res/stbof/files/wdf/` and `ue/gui/stbof/wdf/`), read for the layout
only, October 2026; see [tables.md](tables.md#other-sources). To be checked
against the retail files and, later, against the game's screens in the Let's
Play.

All values little-endian; `i32` unless said otherwise. Names of files are 13
bytes, zero-terminated, with leftovers after the zero (the editor's comments
note one, in `c_tfship.wdf`, where a name runs into the next field); "none"
means no file.

## File

```
0    i32       the file's kind (not known what it means)
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
| 2 | button | `i32` ?, `u8` kind, `i32` repeats when held (0, 1), `i32` repeats a second, normal picture, selected picture |
| 5 | menu | text rectangle (x, y, width, height), two fonts, the menu (`char[13]`, a `.mnu`), left, top, right and bottom pictures, one more file name |
| 7 | slider | `i16` ?, maximum, units, cell width, `i32` ?, cell off, cell on, marker off and marker on pictures, then maximum, units and cell width again |
| 8 | scroll bar | `i32` ?, handle on and handle off pictures, minimum, maximum |
| 12 | list box | `i32` ?, two fonts, 10 column widths, how many columns (1-10) |
| 13 | picture | `i32` ? |
| 14 | text box | text rectangle (x, y, width, height), a font |

Windows hold windows, so a layout is a tree. A widget's place on the screen
is its own x and y plus those of every window it's in, and the editor
draws each picture cut to its widget's rectangle.

A font (48 bytes):

```
0    u8[3]     colour: red, blue, green (in that order, as the editor reads it)
3    i32[7]    not known
31   i32       1: use this font, 0: the game's default
35   char[13]  the font (a .fnt in stbof.res)
```

## Not known yet

- What the file's kind, the button kinds and the unknown values mean.
- How the game finds a widget: presumably by its id, from the executable.
- Whether the colour order is right; the files will tell (white or grey text
  reads the same either way).
