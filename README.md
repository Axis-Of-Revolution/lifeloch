# Lifeloch — v0.10.0

Native Nintendo 3DS game tracker with a standalone offline browser demo. The dark medieval serif interface, raster numerals, WUBRG themes, and existing player layouts remain the visual baseline.

Open `dist/Lifeloch-v0.10-Demo.html`. It contains its artwork, font, audio synthesis, and scripts; no server or external assets are needed. It is a separate browser implementation, not a 3DS emulator. Existing browser saves retain their storage key.

## This revision

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
