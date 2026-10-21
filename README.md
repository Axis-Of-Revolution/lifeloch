# Lifeloch

**Lifeloch** is a two-screen Magic: The Gathering table companion for the Nintendo 3DS family, including New Nintendo 3DS and New Nintendo 3DS XL. It has a native C++ application and a separately implemented, offline HTML demo. The demo is for design and interaction review; it is **not** an emulator or a packaged 3DS application.

> **Build status:** v0.10.10 has been compiled with devkitPro and packaged as [`dist/Lifeloch-v0.10.10.cia`](dist/Lifeloch-v0.10.10.cia). Package structure, title version and content hashes have been checked. **Installation and runtime behavior have not yet been tested on physical 3DS hardware.** See [BUILDING.md](BUILDING.md) for the toolchain and validation record.

## What it does

- Tracks life, poison, energy, experience, commander damage (including partners), monarch, initiative, turns, a game clock/timer, and shared floating mana/storm.
- Supports **2–6 players**, teams/partner-head configurations, configurable starting totals, saved player profiles, seat assignments and game presets. Multiplayer defaults include 40 life and commander damage for three or more players, but settings remain editable.
- Displays players on the upper screen and touch-friendly controls on the lower screen; supports D-pad/button navigation and alternate player orientations for a 3DS placed at the center of the table.
- Offers an oldest-first, scrollable **Game history** with undo/redo. It records life/poison changes and monarch/initiative acquisitions and losses; other counters are not part of that log. A disabled turn counter records turn `0`. New-game confirmation is triggered by recorded history, not every possible unsaved counter change.
- Includes a physical dice/coin tray (coin, d4, d6, d8, d10, d12, d20), animated life digits, low-life pixel flames, and synthesized sound cues.
- Provides Magic-inspired WUBRG backgrounds with slow, hydrodipped color movement, a continuous **1×–2×** rotation-speed setting, and original HOME artwork with restrained pixelation and an animated eight-second wave.

## Visual and interaction principles

Lifeloch uses a **dark medieval/pixel-art style** with an ivory/brass serif treatment. Do not replace the current demo's graphics with generic UI widgets or generated concept art. Player colors and two-color profiles carry into both screens; names and abbreviations use offset shadows. The design prioritizes legible life totals and compact commander/counter groupings in six-player arrangements.

For standard player panels, the player name and recent history occupy the left side, the life total sits at upper-right, and miscellaneous trackers and a fixed **2×2 commander grid** sit at lower-right. The **six-player side-sections** layout has four standard center panels and only two compact side panels; those compact panels move the life total below the name.

## Use

1. Open **New game** and select players, layout, initial totals, profiles and enabled trackers; alternatively resume an existing game.
2. Tap the lower-screen controls to select a player and change life, poison, counters, commander damage, roles, or the turn. A held adjustment repeats.
3. Open **Game history** to review oldest-to-newest events, scroll with touch/arrow controls and use **Undo** or **Redo**.
4. Open **Theme** to choose colors and drag the rotation-speed slider continuously from 1× to 2×.
5. Open **Dice** for the physical tray.

Controller basics: **D-pad** moves menu focus, **A** selects, **B** goes back, **SELECT** toggles button focus, and **START** opens game settings. In-game shortcuts change the current tracker (A/B or Up/Down), switch players (L/R or Left/Right), undo (X), and open history (Y). The lower-screen help page has the latest in-app control legend.

## Run the browser demo

Open [`dist/Lifeloch-v0.10-Demo.html`](dist/Lifeloch-v0.10-Demo.html) in a modern browser; no server is required. The HTML embeds its artwork, font and scripts. The development copy is `tools/demo-v10.html`; **keep both files identical**. Browser saves use the existing `command-deck-v05` local-storage key for compatibility.

## Install on a modded Nintendo 3DS / New 3DS / New 3DS XL

**Prerequisites:** A system already running custom firmware/homebrew capable of installing CIAs, an SD or microSD card, and a successfully built `lifeloch.cia`. *Modifying a stock console is outside this project's instructions.* The steps are identical for the original and New 3DS models once custom firmware is present.

1. Download [`dist/Lifeloch-v0.10.10.cia`](dist/Lifeloch-v0.10.10.cia) using GitHub’s **Download raw file** control, or build it below. Verify its SHA-256 against [`SHA256SUMS.txt`](SHA256SUMS.txt).
2. Copy the downloaded `.cia` to a convenient folder on the console's SD/microSD card.
3. Reinsert the card, open the homebrew **FBI** title manager on the console, browse to the CIA and select **Install and delete CIA** (or **Install CIA** to retain the installer).
4. Return to HOME Menu and launch Lifeloch. Let the app create its save folder at `sdmc:/3ds/command-deck/`. **Do not delete this folder when updating the app**: the old directory name is deliberately retained for save compatibility.

The repository also defines a `.3dsx` build for users who prefer Homebrew Launcher; place the compiled `.3dsx` in an appropriate `/3ds/` app folder on the card. Installing a CIA is not the same as running the HTML demo. Native rendering, sound, persistence, suspend/resume and hardware controls still need verification on actual systems.

## Build from source

Install the **devkitPro Nintendo 3DS toolchain** (devkitARM, libctru, citro2d/citro3d), plus `makerom` and `bannertool`. Set `DEVKITPRO` and `DEVKITARM`, and add the SDK and packaging tools to `PATH`. **Use the makerom v0.19.0 patch and detailed setup in [BUILDING.md](BUILDING.md)**; the unpatched release writes an incorrect minor version.

```sh
make clean
make -j2 cia    # creates lifeloch.elf, lifeloch.3dsx, lifeloch.smdh and lifeloch.cia
```

`make` alone builds the ELF/3DSX. The CIA includes `assets/application.rsf`, the supplied icon/banner and silent banner audio. The title ID remains `000400000F5D3100`; CIA version `170` encodes `0.10.10`. Generated root outputs are ignored; the explicitly versioned CIA in `dist/` is committed. Hardware validation remains outstanding.

## Tests and repository guide

Run the host regressions with `sh tools/test_host.sh`, `sh tools/test_v10.sh`, `sh tools/test_v3.sh`, `sh tools/test_v4.sh`, and `node tools/test_v10_demo.cjs`. With `DEVKITPRO` set, also run `sh tools/test_native_keyboard.sh`. Optional screenshot tests use `npm run test:render` (requires `sharp`). A real-browser click/slider smoke test is `python tools/test_chromium_smoke.py` (requires Python Playwright and Chromium). See **[LLM-CONTEXT.md](LLM-CONTEXT.md)** for architecture and safe editing practices, **[CHANGELOG.md](CHANGELOG.md)** for release history, and **[COPY_AND_BRANDING.md](COPY_AND_BRANDING.md)** for branding and artwork permissions.

The software license does not grant ownership of external/supplied art. The supplied original is `assets/home-source.jpg`; its pixelation and wave are algorithmic, not generative.
