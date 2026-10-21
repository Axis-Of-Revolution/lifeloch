# Lifeloch — v0.10.8

Native Nintendo 3DS game tracker with a standalone offline browser demo. The dark medieval serif interface, raster numerals, WUBRG themes, and existing player layouts remain the visual baseline.

Open `dist/Lifeloch-v0.10-Demo.html`. It contains its artwork, font, audio synthesis, and scripts; no server or external assets are needed. It is a separate browser implementation, not a 3DS emulator. Existing browser saves retain their storage key.

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

## Proposed v0.10.6 panel corrections (previous review)

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

## Previous proposed v0.10.5 upper-screen placement (superseded)

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

## Proposed v0.10.4 refinements (also included)

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

## Retained features

- Creating a profile during setup opens its name and color-pair editor. **Done** assigns the completed profile to the selected seat. Saved-profile name and color edits immediately update linked players, including through undo/redo and save/reload. Deleting a profile detaches the link and preserves the current player's identity.
- Profile initials sit directly on their gradient, with uniform ivory ink and a narrow dark outline following each letter. Default Player E uses a darker, slightly purple black (`#81738e`); F retains its distinct cool grey.
- New games begin with all five WUBRG theme colors selected. Existing game themes and saved presets retain their choices.
- **Game settings** contains commander damage, partners, turn counter, countdown timer, timer controls, and current-time visibility on one screen. The native and demo commander-link control both read **Life linked**.
- The death effect appears at zero life, ten poison in individual play, fifteen poison in either Giant mode, or 21 damage from any one enabled commander. One teammate reaching a commander threshold defeats the whole Giant team. Partner damage remains separate: 20 from each does not trigger commander death. Disabling commander/partner tracking retains the stored values.
- Flames become brighter and cover more of the numeral at one quarter of starting life or lower. Death overrides the flame effect.
- Commander boxes show the three largest source-player totals, in descending order; when partners are enabled both slots contribute to ranking and remain separately displayed. Ties retain player order.
- The upper life-change list starts one row higher, with its **History** heading removed. It still fills the available height and underlines the newest entry. The right arrow is centered on the numeral height and enlarged slightly from v0.9 for readability.
- Lower-screen life and poison history uses bounded columns, row dividers, and separate source details in the selected-player view. Long values and names fit their assigned space.
- Giant teams are separated by continuous lines in upper-screen gutters that follow each seat layout. The lower control panel encloses each team's player buttons in a rectangle. No T1/T2 text labels are needed. Teammates continue to share life and poison; commander damage stays individual.
- The home backsplash is sampled at 320×192 and displayed at 400×240 with nearest-neighbor scaling: pixels are 25% larger. The supplied source artwork and HOME icon are preserved.
- Approved wording is retained: Float, P/E/X counters, profile initials, Commander A/B, Life linked, layout names, rare roll outcomes, Home, and Exit app. Setup mode buttons spell out **2-headed Giant** and **3-headed Giant**.

Existing features remain: live seat-layout/profile changes; no factory presets; D-pad/A/B menu navigation and SELECT button navigation; fast rolling life digits; equal sizing for single- and double-digit life; automatic saves; and the established artwork, title, layouts, and controls.

## Physical dice and coin

The tray is now on the **upper screen**, viewed directly from above. Controls stay on the lower screen. Coin, d4, d6, d8, d10, d12, and d20 use convex meshes. The coin has a thin bevelled rim.

A roll randomizes position, velocity, orientation, and angular velocity. A fixed-step rigid-body solver handles gravity, floor/wall contacts, normal impulses, friction, and rolling resistance. **No result is selected at launch, and there is no final face-alignment rotation.** The result is read only from the resting orientation; d4 uses its upper vertex. A cocked landing is identified and can be rolled again. Sixes and nines have orientation marks. Labels are fitted inside each convex face with an inset margin, including two-digit d10/d20 faces. A three-layer projected silhouette shadow changes with orientation and height; it uses no blur shader or additional physics body.

The solver uses 240Hz substeps, a 1.7 simulation speed, and rising contact resistance near the end to keep motion short. In the native 840-roll regression sample, the longest roll took 2.13 seconds; 20 resting orientations were reported as cocked rather than assigned a result. Timing on actual 3DS hardware remains unverified. Continuous external motion can affect settling.

The native app reads the 3DS accelerometer and gyroscope, with the launch attitude treated as neutral. While the tray is open, deliberate tilt wakes a settled die and continues applying physical forces; returning to level lets it settle and updates the result. A stationary sensor leaves the resting body asleep. Tilt changes the force acting on the body and can change the final result. Yaw rotation applies rotating-tray inertial forces and can wake a settled body even when the device is level. The gyroscope coefficient converts raw yaw readings to radians per second; a small deadband suppresses rest noise. Deliberate ongoing motion keeps the body interactive instead of imposing a result deadline. The browser provides tilt/rotation controls and optional device orientation access; its alpha rotation wraps correctly across 0/360 degrees and stale yaw input expires. Collision and launch sounds are synthesized locally. Sensor axis mapping, feel, and hardware performance still need a physical 3DS check.

## Compatibility and source

Native save format **8** reads formats 1–7. Event kinds distinguish life from poison; legacy records remain life events. The previous save magic, title ID, product code, and SD directory are retained. Earlier five-player layouts and legacy profile records migrate. Active players now retain a saved-profile reference. References are adjusted when profiles are deleted. Legacy snapshots reconnect when a unique saved profile has the same name; ambiguous or missing matches retain their identity until reassigned.

Native source is in `source/` and `include/`. Browser source is `tools/demo-v10.html`, with `tools/dice-physics.js`, `tools/dice-tray.js`, and `tools/hit-audio.js`. Both versions share the generated glyph atlas and mesh data. `tools/sync_demo_atlas.py` embeds the browser modules and home artwork into the demo.

```sh
python3 tools/make_assets.py
# Run only after editing mesh generation:
python3 tools/generate_dice_meshes.py
sh tools/build_demo.sh
sh tools/test_host.sh
sh tools/test_v3.sh
sh tools/test_v4.sh
sh tools/test_v10.sh
```

Optional raster review requires Node and `sharp`:

```sh
npm run test:render
```

This creates home/menu screens, all sixteen layouts, team partitions, profile/history views, and physical roll frames in `qa/`. `tools/render_native_v10.cpp` exports corresponding host-rendered C++ snapshots. These checks are logic tests and SVG renders, not real browser-engine or 3DS tests.

## Native build status

No CIA or native executable is included. devkitARM, libctru/citro2d build dependencies, makerom, and bannertool are unavailable in this environment. Compilation, installation, accelerometer response, performance, audio, keyboard entry, SD persistence, and suspend/resume need hardware verification.

With the devkitPro 3DS toolchain and packaging tools installed and `DEVKITPRO` / `DEVKITARM` configured:

```sh
make
make cia
```

Outputs are `lifeloch.3dsx` and `lifeloch.cia`; CIA version field 160 corresponds to 0.9.0. `make cia` regenerates the compiled banner from the current image.

See `COPY_AND_BRANDING.md` for approved terminology and the copy audit. The supplied artwork is retained as `assets/home-source.jpg`; the software license does not relicense that artwork.
