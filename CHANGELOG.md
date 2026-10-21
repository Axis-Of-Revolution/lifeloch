# Lifeloch changelog

This file preserves the chronological design/implementation notes originally kept in README.md. Some entries describe superseded approaches; **the most recent entries and current code take precedence**. No earlier features are reinstated solely because they appear here.

## v0.10.10 native CIA build — 2026-10-20

- Compiled GitHub source `d3cbf0ecc90c45cab9dac58ada06cb158dc13412` with devkitPro/devkitARM GCC 16.1.0, libctru 2.7.0, citro2d 1.7.0 and citro3d 1.7.1; published `dist/Lifeloch-v0.10.10.cia` with SHA-256 provenance. App source and demo behavior are unchanged.
- Set packed CIA version to 170 (`0.10.10`), made `make clean` remove the generated banner, and allowed versioned distribution CIAs in Git.
- Documented and included a makerom v0.19.0 build patch: tool-version macros collided with title-version array indices and discarded the requested minor version. The final package reports the correct version; content, ExHeader and ExeFS hashes pass.
- Corrected stale keyboard regression expectations to the current eight-character profile-name limit. All host, v10, v3/v4, keyboard adapter and demo regression suites pass.
- Updated installation, build, release and contributor documentation. This is a compiled homebrew package for custom firmware; installation, performance and runtime behavior remain untested on physical hardware.

## v0.10.10 input stability, animation continuity and documentation

- Separated documentation into an installation/use-focused `README.md`, full historical `CHANGELOG.md`, and architecture/editing guide `LLM-CONTEXT.md`. Added code comments around history gestures, save format changes and input state.
- Added an optional real Chromium smoke test checking actual mouse clicks through ambient background frames and speed-slider interaction.
- Lower-screen SVG changes now update the animated background image **in place**; stable generated IDs prevent remounting controls during ambient animation.

### Interaction and animation corrections

- **Browser click reliability:** The 50 ms ambient animation no longer replaces
  identical lower-screen SVG markup. Mouse-down controls stay attached until
  click completion; rapid animation on the upper screen does not steal clicks.
- **Stable theme adjustment:** Theme phase now accumulates continuously from
  elapsed time and the prior frame's speed in both renderers. Dragging speed
  changes rotation rate without teleporting the color field. Browser slider
  dragging persists the new value on release instead of on every move.
- **Home artwork framing:** Original (non-generated) home artwork is center-
  cropped with 6% zoom in both renderers, eliminating the dark side slivers
  exposed by the horizontal wave. The eight-second / 1.875-pixel wave remains.

## v0.10.9 fluid-marble theme motion

- Theme now has a **continuous touchscreen slider from 1.00× to 2.00×** rather than five notches. The in-app rail is framed to match the existing dark medieval controls; adjacent − and + buttons move by 0.025× for D-pad/controller access. The native setting is preserved to one-thousandth precision in save format **v11**, with migration from the former v10 notches (values above 2× are capped at 2×). Browser local saves migrate in place.
- The background uses a more pronounced flowing **hydrodipped / marbled color-boundary field**: wavy, multi-scale coordinate distortion and a narrow transition between the two nearest rotating pigments, replacing the previous inverse-distance blurred blend. Native and browser share the boundary and distortion math. The browser uses a smooth 160×120 rasterized bitmap rather than square SVG tiles, updated at 50ms intervals (instead of 750ms); 3DS gradient patches and cached wave samples remain GPU-friendly, but console frame rate needs hardware validation.
- The original home artwork remains unchanged: the horizontal wave now loops every **8 seconds** at **1.5× the previous amplitude** (1.875px vs 1.25px), in both implementations.
- Taking monarch/initiative is now colored like life gain, and losing either role like life loss, within the Game history view. History recording and undo/redo behavior are unchanged.

## v0.10.8 adjustable theme motion

- The Theme submenu adds a five-position notched speed slider: 1×, 1.5×, 2×, 2.5×, and 3×. It defaults to the original 1× and persists globally in native save v10 or existing browser saves.
- The gradient centers and boundary ripples move at the selected speed in both the browser and 3DS renderers, with the same 125-second base orbital period.
- The home-artwork wave cycle decreases from 18 to 12 seconds (50% faster oscillation) in both renderers; amplitude and pixelation remain unchanged.
- The history/role fixes from this revision are included, as are the provisional panel styling changes.

## v0.10.8 history fixes and v0.10.7 styling refinements

- Game-history events record **turn 0** when the turn counter is disabled, even if
  a previously configured turn number is nonzero. When the turn column is shown,
  historic missing values also render as `0` instead of `?`; previously saved
  data is left intact. Turn tracking ON continues to use the actual turn.
- Taking, relinquishing, and transferring Monarch or Initiative now append
  history events. Transfers generate **Loses role** then **Takes role**, using the
  actual former and new owners; both entries share one undo/redo action.
- No changes were made to legacy long-name display or the history-based
  new-game confirmation scope, per the design decision.

### Previous v0.10.7 styling refinements

- Colored upper-screen player names and their lower-screen control-panel counterpart
  now use a subtle pixel-compatible offset dark drop shadow while retaining any
  solid or two-color profile gradient. Neither the font nor layout geometry changes.
- Upper-screen profile abbreviations and the player selection abbreviations
  in the lower control panel use displaced shadows instead of stroke outlines.
  The shadow follows the glyphs, without an opaque backing rectangle.
- Poison, energy, and experience values are **right-aligned within their own
  allotted tracker cells**. Monarch and initiative icons are anchored to the
  right edge of their allocated cell, including when only one icon is present.
- The existing two-thirds-width tracker band and 2×2 commander grid remain
  unchanged, as do the six-player compact-side exception and the game's style.
- The browser and native C++ use matching placement offsets and glyph ink;
  automated native and browser tests cover the updated design language.

The deliberately retained scope is life/poison changes and monarch/initiative
transitions; other counters are not added to game history.

This version includes the previously reviewed upper/lower player-panel styling.

## v0.10.6 panel corrections (previous review)

The six-player **Side sections** arrangement now has **four central standard
panels (111×99 each)** and **two smaller side panels (76×125 each)**. The
side players rotate to local dimensions 125×76, and the side slots are
vertically centered so that they do not dominate the center players.

In the four central panels, the life total remains at upper-right, commander
and trackers at lower-right, and history beneath the player identity at left.
Only the two compact side panels move the life total **below the name**; the
life total remains **right-justified**, and history remains **left-justified**.
Compact side player names may extend beyond half the panel width and have a
minimum displayed font size of 8; all name displays still limit to eight
codepoints and never scale above their original default.

Names now use their player/profile colors, including matching two-color
identity gradients, instead of a generic ivory label. Commander grid text is
centered within symmetric 2×2 cells using a more conservative font cap and
extra inter-cell spacing.

This layout is included in the v0.10.8 release. The compact
side panels leave some themed background visible above and below them;
confirm this sizing using the screenshot before approval.

## v0.10.5 upper-screen placement (superseded)

This earlier layout review has been incorporated into the current release. It continues the proposed
v0.10.4 history/marker changes, but supersedes its upper-screen player-panel layout.

- **Standard panels:** player identity and an eight-character displayed name fit
  inside the left half of the panel. The player name font never grows above its
  regular size, scaling down only if needed. Life totals are right aligned,
  start at the top 5-pixel margin, and occupy the right half of the panel.
- **Mini-history:** newest life/poison changes appear left justified immediately
  under the player name, within the left third of the panel. The font is larger
  than in v0.10.4. Equal 6-pixel left and right outer margins are preserved.
- **Trackers and commander damage:** both use the right two-thirds; misc counters,
  monarch, and initiative appear directly above the commander box. Commander
  damage is fixed to the bottom of the panel with two text rows and two columns.
  The four greatest nonzero sources are shown in row-major order; text is
  dynamically reduced to fit each half-width cell, never requiring a third row.
- **Life-space protection:** the normal panel preserves at least half of its
  vertical extent for the upper life-display region. The rendered numerals retain
  their original aspect ratio and may be smaller than half the panel's height
  where the half-width constraint prevents taller digits.
- **Six-player side sections exception:** all six panels place the life total
  below the name. The bottom tracker/commander bands extend across the card,
  which shortens the nearby mini-history if necessary. Commander remains 2x2.
- **Profile edits:** new or renamed player profiles are limited to eight UTF-8
  characters. Previously stored longer names remain valid in saved games and
  are visually truncated to eight in the upper-screen player panels.

## v0.10.4 refinements (also included)

- Game history controls use Undo and adjacent Redo, pixel up/down arrows and Menu.
- The empty state reads exactly "No game history recorded yet."
- Monarch and initiative acquisitions are recorded as undoable history events.
- Commander damage has no heading; miscellaneous trackers sit above it.

## v0.10.3 refinements

- Main life-total numerals now have a crisp dark silhouette, subtle offset drop shadow, and shallow serif edge depth like the HOME LIFELOCH title. Defeated life totals use a lighter grey-blue to remain legible above the waving skull-and-crossbones without losing the subdued appearance.
- **Game history** is now a single oldest-first event table in both All and individual views. The optional **Turn #** column is shown only when turn tracking is enabled, and older events without associated turn metadata display `?`. The table has a right-edge scrollbar and supports direct touch dragging, scroll-wheel input in the browser, D-pad controls, and compact pixel-arrow buttons in place of Newer/Older. The reclaimed space adds **Redo** alongside Undo and Menu.
- The upper display's life-change history column no longer has a vertical divider; it remains visually separate because of spacing and text styling, with no commander-border seam to manage.
- Theme colors slowly orbit their center on a 125-second cycle while subtle low-amplitude sine waves move blending boundaries. The browser updates cached color tiles at 750-ms intervals and the native renderer caches orbital geometry to minimize CPU overhead.
- The original algorithmically pixelated HOME art receives a subtle horizontal ripple (1.25 pixel amplitude, 18-second cycle). Native uses narrow UV texture bands; the standalone browser uses the same cropped strips. The underlying original artwork remains unchanged; no generated imagery is used.

## v0.10.2 refinements

- Restore the original pixel-art language for low-life flames: animated square-cell sprites inside the numerals, looping seamlessly every 2.4 seconds. Fire rises higher and burns brighter at or below one quarter of the configured starting life; defeat still takes precedence.
- Miscellaneous counters and monarch/initiative sprites share the left-side commander width, with aligned baselines, matching card margins, and an unobstructed right history column.
- The commander damage box's right border shares the same one-pixel vertical line as the life-history divider.
- Rename the history page **Game history**. With turn tracking enabled, history events are grouped beneath explicit turn separators in both all-player and single-player views. Save format version 9 records event turns while retaining v1–v8 loading; old events are labeled *Earlier turns* rather than assigned invented turns.
- Slightly pixelate the original home-screen illustration using algorithmic resampling only: downsample to 200×120 and enlarge with nearest-neighbor sampling. The original image remains unchanged at `assets/home-source.jpg`; browser and native apply the same pixel scale.

## v0.10.1 refinements

- Low-life numerals now carry layered, rising flame tongues with ivory/amber cores. Both native and browser visuals loop every 2.4 seconds with matching flame colors and no abrupt animation-phase resets. The browser keeps the animation in the SVG fill instead of recreating a timed CSS style each render.
- Large life losses have a harder critical-hit attack, a deeper decaying body strike, a short second transient, and a faster steel edge. Both renderers share the same deterministic 22.05 kHz synthesis and a soft-limited peak.
- The commander damage strip is narrower beneath life totals, with tighter source-cell spacing. The history column reaches the bottom counters regardless of the commander strip; compact seats allow a small intrusion where necessary.
- Choosing **Create a new game** with existing life/poison history immediately asks for confirmation, before setup changes or the previous game is discarded. Cancel returns to the existing game. Confirmed setup avoids a second prompt when starting.
- Changing the setup player count to three or more now turns on commander damage automatically and sets starting life to 40. Changing back to two turns it off and restores the 20-life default. Manual overrides and saved presets remain available.

## v0.10 baseline

- Profile abbreviations use one ivory fill with a thin dark glyph outline, including white/black gradients. There is no small black backing rectangle or split-letter fill.
- Team dividers have overlapping square joins. All dark edging is painted before the light lines so corners and T-junctions stay continuous.
- Small life losses use a damped, downward-pitched thud. Large losses use a quick blade swish, sharp impact, and metallic ring. Native and demo hit samples use matching deterministic synthesis.
- Life additions and subtractions within a two-second gap form one net history event. Exact reversals remove empty events. Switching recipient, counter, or commander source/slot starts a new event; undo/redo also ends the current batch.
- Poison has the same grouped history and undo behavior. Purple **P** entries appear in both the player cards and the renamed **Life and poison history** page. Net additions of three or more in the event use the critical hit, smaller additions use the thud, and removals are silent. Giant teammates share one poison event.
- Upper poison/energy/experience labels are 20% larger with width fitting and a reserved bottom band. Adjustment buttons flash for 170 ms and stay lit while held, including hardware shortcuts.
- Gyroscope yaw now rotates the physical tray; accelerometer tilt remains active, including after a die has settled. Both input paths change the physical motion and the resting face determines the result.

