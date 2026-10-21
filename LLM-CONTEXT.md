# Lifeloch — codebase context for coding assistants

**Read this before making changes.** Current release target: `v0.10.10`, the direct descendant of published `v0.10.9`; use GitHub `main` as the canonical source only after its updated commit is verified. The offline browser demo and native Nintendo 3DS implementation are **separate applications**, not two skins on shared executable code. Features, interactions and visuals must be deliberately ported to both.

## Product constraints (do not reinterpret)

- 2–6 players. All layouts and rotations matter; users routinely test **4-player** and **6-player side sections**. Six-player side sections = **2 compact outer seats, 4 regular central seats**.
- Standard player card: profile name top-left (max eight visible characters; shrink only, do not enlarge), mini-history under it, life number top-right; trackers and 2×2 commander damage bottom-right. Compact side seat exception: move life number below player name, keep it right-aligned and leave history on left.
- Commander box remains **exactly 2 rows × 2 columns** where enabled. Center entries inside cells and keep padding symmetric. Do not add a heading or let the box grow upward into life.
- Colored player names and abbreviations follow profile color/gradient with **offset shadows**, not opaque labels or heavy strokes. Pixelated low-life flames intensify at ≤25% of starting life. Maintain the dark medieval serif/pixel-art UI already implemented; the demo is the visual reference.
- Theme is the slow fluid/marbled WUBRG gradient with **continuous 1–2×** rotation speed. Never compute animated phase as absolute time multiplied by the *new* speed; integrate elapsed time with the *previous* speed. Home artwork comes from the supplied image, pixelated algorithmically, with an 8-second horizontal wave (1.875 px) and ~6% overscan to avoid blank side edges. No generative replacement art.
- Game history is **oldest first**; tracks life/poison and monarch/initiative gain/loss, not all other counters. Turn-disabled events are attributed to `0`; old unset turns display `0`. Supports touch and D-pad scrolling, Undo next to Redo. New-game confirmation triggers when history exists.
- Preserve existing saved profiles, presets and save migrations; do not silently rename `sdmc:/3ds/command-deck` or browser local-storage key `command-deck-v05`.

## Repository map

| Location | Responsibility |
| --- | --- |
| `include/model.hpp`, `source/model.cpp` | Domain state, profiles/presets, game actions, counter batching, history events, undo/redo and validation |
| `include/ui.hpp`, `source/ui.cpp` | Native page navigation, touch/D-pad command dispatch, 400×240 top/320×240 bottom layout, player panels, history display |
| `include/theme.hpp`, `source/theme.cpp` | WUBRG palette, animated color-field sampling and native theme contrast; coordinates/time-speed math mirrored in HTML |
| `include/pixel.hpp`, `source/pixel.cpp`, `include/medieval_atlas.hpp` | Raster glyphs, pixelated flames and numerical rendering |
| `source/main_3ds.cpp` | devkitPro/citro2d entry point, GPU artwork upload and wave strips, 3DS hardware input, frame loop and autosaving |
| `source/storage.cpp`, `include/storage.hpp` | Versioned binary saves, checksums, migration and backup/recovery logic; current format v11 |
| `source/dice.cpp`, `include/dice.hpp`, `include/dice_meshes.hpp` | Physics dice and coin simulation |
| `source/audio.cpp`, `include/audio.hpp` | Native synthesized sound effects |
| `source/layout.cpp`, `include/layout.hpp` | Layout rectangles, seat orientation and team dividers |
| `tools/demo-v10.html` | Editable **all-in-one** HTML reference (embedded artwork, SVG UI, Web Audio, dice simulation); copy to `dist/Lifeloch-v0.10-Demo.html` after edits |
| `tools/dice-tray.js`, `tools/dice-physics.js`, `tools/hit-audio.js` | Modular source snippets/resources consumed by the demo build; inspect before changing embedded counterparts |
| `assets/home-source.jpg`, `tools/make_assets.py`, `include/home_art.hpp` | Supplied home source art, deterministic pixelation/tooling and native baked art pixels |
| `tests/`, `tools/test_*.sh`, `tools/test_v10_demo.cjs` | Host/native-state tests, save regression, browser-DOM logic and layout tests |
| `Makefile`, `assets/application.rsf`, `BUILDING.md` | devkitARM build, CIA metadata and verified build procedure (not hardware verified) |

## Event pipeline and invariants

`source/main_3ds.cpp` receives physical touch/keys → `App` (`source/ui.cpp`) maps input to `Command` → `Model` changes `State`/`Game` and updates history → `App::render` sends top/bottom screen draw commands to `Renderer`. Model mutations increment `revision`; native storage watches this for autosave. A press/hold gesture can batch life or poison changes into a net history event; **do not bypass `Model`** or break gesture boundaries. Undo/redo snapshots exclude ongoing timers/elapsed time.

The native renderer uses the platform texture/graphics API, while HTML uses `topScreen()`, `lowerScreen()`, `render()`, and delegated SVG `data-a` actions in one embedded script. **Do not replace the entire lower-screen SVG every ambient animation frame**; doing so interrupts mouse `click` after `pointerdown`. Update the background image in place, or use an equivalent DOM-preserving strategy. Keep the slider draggable with pointer capture; only persist on drag completion.

Themes: phase is elapsed-time integrated at the previous speed. Browser image frames can be cached by mask/width/frame while controls must not be remounted. Avoid multiplying time directly by slider values. The native variant caches warp arrays and evaluates a spatially warped multi-pigment field. Check parity with browser formulas when altering wave or orbit constants.

Native binary serialization uses explicit version gates. For schema changes, update encode/decode, defaults, validity checks, migration fixtures and round-trip tests; never overwrite existing saves without compatibility review. Avoid changing the browser storage key unless an explicit migration is included.

## Testing and release procedure

1. Read current `README.md`, `CHANGELOG.md`, source headers and affected modules; inspect existing behavior before editing.
2. Change the native and HTML implementations in tandem; keep `tools/demo-v10.html` byte-identical to `dist/Lifeloch-v0.10-Demo.html`.
3. Run `sh tools/test_host.sh`, `sh tools/test_v10.sh`, `sh tools/test_v3.sh`, `sh tools/test_v4.sh`, `node tools/test_v10_demo.cjs`; optional visual snapshots via `npm run test:render`. Use optional `python tools/test_chromium_smoke.py` with Python Playwright/Chromium to verify **real mouse clicks** while the theme animates and drag speed without position jumps.
4. Visually check at least 2, 4, 6 default and 6 side-sections panels, both screens, and history scrolling. Run `make`/`make cia` with devkitPro when available; do **not** claim native compilation or hardware validation based on host tests.
5. Update version strings, `CHANGELOG.md`, `BUILD_STATUS.txt`, and `RELEASE_INSTRUCTIONS.txt`; regenerate the source ZIP with `tools/package_v10.py`. Source/demo ZIPs remain source-only. Keep the current versioned CIA in `dist/`, regenerate `SHA256SUMS.txt` after updates, and follow `BUILDING.md` for package verification; never include stale binaries.
6. If authorized to publish, create a single GitHub commit (branch `main` currently used), verify its head and the exact published file contents. Never report a successful publication from a local commit alone.

## Known limits and deliberate choices

- v0.10.10 CIA is compiled with devkitPro and its package hashes/title version are checked; physical original/New 3DS hardware is still untested. Validate sound, touch, GPU load, SD save safety and suspend/resume before calling it production-ready.
- makerom v0.19.0 needs the supplied version-index patch; see `BUILDING.md`. The profile keyboard adapter test must expect Model’s eight-character name limit, even though the OS keyboard permits 16.
- User explicitly chose **not** to extend new-game confirmation to nonhistorical progress or log every counter change.
- User specifically rejected old draft visuals, extra headings, unapproved history copy, excessive name enlargement and nonpixelated flames. Treat approved current demo as the authoritative style.
- Documentation is a map, not an implementation source. When it conflicts with code, inspect tests and ask or use the latest explicit user instruction.
