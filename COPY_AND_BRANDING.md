# Lifeloch — copy and artwork

The title is **Lifeloch**, displayed as **LIFELOCH** on the home screen. The subheading is **A Magic: The Gathering Game Tracker**. No placeholder text or artwork remains.

Menu headings use sentence case. The requested **Turn Counter / Clock** setup button and **LIFELOCH** title retain their capitalization. ON/OFF, compact scoreboard labels, and mana codes use capitals.

## Approved terminology

| Label | Use |
| --- | --- |
| Float | Control-panel button; opens Floating mana tracker. |
| P / E / X | Poison, energy, and experience counters. |
| Two-letter profile initials | Player identity marks directly on color gradients. |
| Commander A / Commander B | Separate partner commander slots. |
| Life linked | Links commander damage adjustments to life adjustments. |
| Existing layout names | Retained unchanged. |
| COCKED / ROLL AGAIN; ON EDGE / FLIP AGAIN | Physical outcomes without a readable flat result. |
| Home / Exit app | Return to the opening menu or close the app; saves are automatic. |

“Shared team identifiers” previously referred to T1/T2 abbreviations. They have been removed. Giant teams are now separated by lines following the upper-screen card boundaries; lower-screen player selectors have a rectangle around each team. Setup explicitly names 2-headed Giant and 3-headed Giant modes.

The History heading inside player cards is removed. **History** remains the lower control-panel button; **Life and poison history** is the dedicated page heading. Game settings has all its controls on one screen.

There are no outstanding placeholder or terminology review items from the supplied list. User-entered profile and preset names remain user copy.

## Artwork and editing

The supplied artwork appears behind the home title and supplies the HOME icon and banner. Runtime display sampling uses 320×192 enlarged to 400×240 with nearest-neighbor filtering, giving 25% larger pixels. This changes the home backsplash rendering, not the source artwork or icon crop.

`tools/make_assets.py` prepares the full 400×240 home asset, 48×48 and 24×24 HOME icons, 256×128 banner, and native RGB565 pixels. Native labels are in `source/ui.cpp`; demo labels are in `tools/demo-v10.html`. Rebuild the self-contained demo with `sh tools/build_demo.sh`.

No CIA has been compiled in this environment; the final native build still requires the 3DS SDK and hardware verification.
