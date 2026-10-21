# Lifeloch — proposed v0.10.6 player-panel review

**Provisional review only — not committed to GitHub.** This build derives from the v0.10.5 preview.

## Six-player side sections

- Four central player panels use the standard arrangement: name and history on the left, right-aligned life at top, trackers above bottom-right commander.
- Only the two **compact side players** use the exception: right-justified life moves below the name, history remains on the left, and trackers plus commander remain right-aligned.
- Geometry is mirrored between C++ and HTML: central four panels are each **111 × 99** physical pixels; two side panels are **76 × 125** physical pixels, rotated to local 125 × 76. Both side panels are smaller by area than every central panel. Compact panels are vertically centered, leaving intentional background space above and below them rather than taking height from the central seats.
- Name text is truncated for display to 8 Unicode codepoints. Compact side names have an 8px minimum size and may use more than half their card width; standard panels retain the half-width name rule. Names use the same player/profile color or ink gradient as the lower-screen controls.
- Commander damage remains a fixed **2 × 2** grid; four strongest nonzero sources displayed in two rows. Text is centered horizontally in each equally padded cell, its size limited to 9.5 native / browser design units and reduced for long source labels, including partners.

## Review focus

Check the six-player side-sections arrangement, in particular the amount of unused background above/below the small side seats and life readability in compact seats. This is a geometry tradeoff: taller side panels would once again dominate the four central seats. No changes have been published to the private GitHub repository.

## Validation

Native C++ host and browser tests, screenshot renders from Chromium, 248 other layout cases. Native Nintendo 3DS hardware has not been tested.

## v0.10.7 text and tracker placement

Retain v0.10.6 player-panel proportions and the two compact side seats.
Name text gains a displaced dark raster shadow without changing its identity ink;
initials lose the outlining stroke and use the same shadow language.
The existing tracker band remains right-aligned to the panel and each sub-cell
now anchors its contents at its own right edge, including role icons.
