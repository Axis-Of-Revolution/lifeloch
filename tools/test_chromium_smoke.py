#!/usr/bin/env python3
"""Optional real-browser input smoke test; requires Python Playwright + Chromium.

Unlike the Node DOM harness, this exercises pointerdown -> mouseup and verifies
that an ambient animation frame does not remount the clicked SVG button.
"""
import asyncio
from pathlib import Path
from shutil import which

from playwright.async_api import async_playwright

ROOT = Path(__file__).resolve().parents[1]
DEMO = ROOT / "dist/Lifeloch-v0.10-Demo.html"


async def main():
    async with async_playwright() as playwright:
        browser = await playwright.chromium.launch(
            headless=True, executable_path=which("chromium") or which("google-chrome")
        )
        page = await browser.new_page(viewport={"width": 1000, "height": 950})
        errors = []
        page.on("pageerror", lambda error: errors.append(str(error)))
        # Self-contained HTML supports set_content without local-file access.
        await page.set_content(DEMO.read_text(encoding="utf-8"), wait_until="load")
        await page.wait_for_timeout(400)
        assert await page.evaluate("window.LifelochPreview.page") == "home"

        # Verify that press targets survive multiple background animation frames.
        await page.evaluate('window.__testTarget=document.querySelector(\'#bottom [data-a="new"]\')')
        await page.wait_for_timeout(300)
        assert await page.evaluate('window.__testTarget===document.querySelector(\'#bottom [data-a="new"]\')')
        await page.locator('#bottom [data-a="new"]').first.click()
        assert await page.evaluate("window.LifelochPreview.page") == "setup"

        for action, expected in [("options", "options"), ("back", "setup"),
                                 ("seats", "seats"), ("back", "setup"),
                                 ("theme", "theme")]:
            await page.wait_for_timeout(180)
            await page.locator(f'#bottom [data-a="{action}"]').first.click()
            assert await page.evaluate("window.LifelochPreview.page") == expected

        # Sample changing a fractional speed while the field animates.
        bounds = await page.locator("#bottom").bounding_box()
        x = bounds["x"] + bounds["width"] * 170 / 320
        y = bounds["y"] + bounds["height"] * 175 / 240
        await page.mouse.move(x, y)
        await page.mouse.down()
        await page.mouse.move(x + bounds["width"] * 0.08, y, steps=4)
        await page.mouse.up()
        speed = await page.evaluate("window.LifelochPreview.state.themeSpeed")
        assert 1.0 < speed < 2.0
        assert not errors, errors
        print("PASS: actual Chromium mouse clicks survive background animation; theme slider drags")
        await browser.close()


if __name__ == "__main__":
    asyncio.run(main())
