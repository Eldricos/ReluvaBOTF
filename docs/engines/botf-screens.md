# Birth of the Federation: the game's screens

How each of the game's screens is put back together, from its `.wdf`
layout ([wdf.md](../formats/botf/wdf.md)), the manual, `lexicon.dic`
([lexicon.md](../formats/botf/lexicon.md)) and recordings of the game.

## Game setup ("Configuration")

`src/engines/botf/setup_screen.cpp`; the viewer's last page shows it
working. Sources: `gmesetup.wdf` and `opening.wdf`; the manual's chapter 5
("Single Player Game", First Edition, May 1999) for what each control does,
its choices and their defaults; a recording of the game ("Let's Play Star
Trek Birth of the Federation, UFP, p1", the screen at 0:57-1:17), October
2026, for how it looks.

- **Drawn over the main menu.** In the recording the setup screen is a panel
  over the main menu (`opening.wdf`, whose buttons stay visible on the
  right): its background's magenta leaves the menu showing.
- **The controls**, matched to the manual by their order on the screen, and
  checked against the recording:

  | Control | Widgets (button, text) | Choices (default first) |
  |---|---|---|
  | Each empire's civilization level, Cardassian to Romulan | 9223/9224, 9225/9226, 9227/9228, 9229/9230, 9231/9232 | Early, Developed, Expanded, Advanced, Beginning. Shift-click: every empire takes this level, or the next if they all have it |
  | Minor races | 9207/9208 | Many, None, Few, Some |
  | Difficulty | 9209/9210 | Normal, Hard, Impossible, Simple, Easy |
  | Strategic timer | 9211 (less), 9212 (more), 9213 | None; the others aren't known yet |
  | Tactical timer | 9214, 9215, 9216 | the same |
  | Random events | 9217/9218 | On, Off |
  | Tactical combat | 9219/9220 | Manual, Automatic |
  | Victory conditions | 9221/9222 | Domination, Vendetta (Team Play is for multiplayer games) |
  | Galaxy shape | 9234/9206, picture 9204 (`cc_gsir`, `cc_gsel`, `cc_gsrg`, `cc_gssp`) | Irregular, Elliptical, Ring, Spiral |
  | Galaxy size | 9233/9205, picture 9203 (`cc_gzsm`, `cc_gzmd`, `cc_gzlg`) | Medium, Large, Small |
  | Accept, Cancel | 9201, 9202 | |

  The recording settles which galaxy picture is which: the shape is the
  upper one, the size the lower.
- **The text.** Each value is drawn in its text box's font, aligned as the
  font says (its first flag right, its second centred, neither left; found
  by comparing the recording with the files), in the middle of the box's
  height. Drawn this way over a still of the recording, every value lands on
  the game's own, letter for letter.
- **The words** are `lexicon.dic`'s, but the screen numbers the
  civilization levels ("2 EARLY"; the entry is "EARLY") and shows the
  timers' "None" in capitals. With the English `lexicon.dic`, drawn over the
  still, they match it as before.
- **Colours.** The glyphs are shades of grey and the game colours them. The
  colours are measured from the recording (the brightest pixels of the
  text): blue (51, 90, 140) for the preferences and galaxy, green (120,
  180, 150) for the civilization levels, pale green (176, 202, 153) for the
  timers. Most of these fonts say "the game's default", so the file doesn't
  hold them; the timers' own colour (`00 ff 00`) doesn't explain theirs.
- **Not known yet:** the timers' other choices; whether the game shows
  pressed buttons with their "selected" picture while held (the viewer
  does); what the main menu's buttons look like while the panel is open (in
  the recording they look dimmed).
