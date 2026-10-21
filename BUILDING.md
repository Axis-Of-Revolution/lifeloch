# Building and checking the native CIA

The repository contains `dist/Lifeloch-v0.10.10.cia`, built from app source
commit `d3cbf0ecc90c45cab9dac58ada06cb158dc13412`. The build publication changes
packaging, documentation and a stale test expectation; `source/`, `include/`,
artwork and both demo HTML files are unchanged.

The package has been compiled and inspected, but **has not been installed or
run on physical original/New 3DS hardware**. It requires an already modded
console with custom firmware. Preserve `sdmc:/3ds/command-deck/` when updating.

## Toolchain used

Linux x86-64, official devkitPro packages obtained from
`https://pkg.devkitpro.org/packages/` and its `linux/x86_64/` repository;
package SHA-256 values were checked against the repository databases. The SDK
was extracted to a workspace-local directory without changing system packages.
For a normal supported installation, use [devkitPro's setup instructions](https://devkitpro.org/wiki/Getting_Started)
and the `3ds-dev` package group.

| Package | Version |
| --- | --- |
| devkitarm-gcc | 16.1.0-1 |
| devkitarm-binutils | 2.46.0-1 |
| devkitarm-newlib | 4.6.0.20260123-5 |
| devkitarm-crtls | 1.2.6-1 |
| devkitarm-rules | 1.6.0-4 |
| libctru | 2.7.0-1 |
| citro2d | 1.7.0-1 |
| citro3d | 1.7.1-2 |
| 3dstools | 1.3.1-3 |
| general-tools | 1.4.4-1 |

Packaging/inspection tools:

- [bannertool 1.2.0](https://github.com/diasurgical/bannertool/releases/tag/1.2.0), Linux x86-64 binary.
- [makerom 0.19.0](https://github.com/3DSGuy/Project_CTR/releases/tag/makerom-v0.19.0), built from the tagged source with the patch below.
- [ctrtool 1.3.0](https://github.com/3DSGuy/Project_CTR/releases/tag/ctrtool-v1.3.0), Linux x86-64 binary, used to inspect the finished CIA.

## Required makerom v0.19.0 fix

The upstream `src/version.h` defines `VER_MAJOR`/`VER_MINOR` as the tool's version
numbers. `src/user_settings.c` includes that header after the enum used for
indexing the three-element title-version array. As a result, the minor index
becomes 19 instead of 1. The unmodified release produced `0.0.10` when given
`-ver 170` or `-major 0 -minor 10 -micro 10` in this build.

[`tools/patches/makerom-v0.19.0-version-indices.patch`](tools/patches/makerom-v0.19.0-version-indices.patch)
renames only the tool-version macros and the banner's references to them,
leaving the title-version enum intact. Build it with the upstream Makefile:

```sh
# Run from the Lifeloch checkout. These tool sources live outside the repo.
LIFELOCH_ROOT="$PWD"
mkdir -p ../lifeloch-build-tools
cd ../lifeloch-build-tools
curl -fL https://github.com/3DSGuy/Project_CTR/archive/refs/tags/makerom-v0.19.0.tar.gz -o makerom-src.tar.gz
tar -xzf makerom-src.tar.gz
cd Project_CTR-makerom-v0.19.0/makerom
patch -p1 < "$LIFELOCH_ROOT/tools/patches/makerom-v0.19.0-version-indices.patch"
make -j2 deps
make -j2
export PATH="$PWD/bin:$PATH"
cd "$LIFELOCH_ROOT"
```

`make deps` uses the dependency sources bundled in that tag. This patch is a
packaging-tool correction; it does not modify Lifeloch's application behavior.
Recheck the embedded title version if replacing makerom with another release.

## Build

Set the SDK location to your actual installation; ensure `bannertool` and the
patched `makerom` above are executable and on `PATH`:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITARM="$DEVKITPRO/devkitARM"
export PATH="$DEVKITARM/bin:$DEVKITPRO/tools/bin:$PATH"
make clean
make -j2 cia
```

Outputs are `lifeloch.elf`, `lifeloch.3dsx` (with embedded SMDH), `lifeloch.smdh`,
and `lifeloch.cia`. `make clean` also removes the generated `assets/banner.bnr`.
`CIA_VERSION=170` encodes `0.10.10` as `(major << 10) | (minor << 4) | micro`.
The existing title ID `000400000F5D3100`, product code `CTR-P-LFLC`, RSF access
permissions, save path and supplied artwork are preserved.

The native build completed without errors or compiler warnings. GCC emitted
informational notes about parameter passing having changed in GCC 7.1.
The host build of upstream makerom emitted an existing `sprintf` warning in
`utils.c`; no change to that unrelated tool code was made.

## Check the package

```sh
ctrtool --verify lifeloch.cia
sha256sum dist/Lifeloch-v0.10.10.cia
```

For the committed CIA:

- Size: **477120 bytes**.
- SHA-256: `bf341da308d1964e192a4651921c9052a080d32642dd76c8e03eb0775873ffee`.
- Ticket/TMD title ID: `000400000f5d3100`.
- Ticket/TMD title version: **0.10.10 (170)**.
- Content, ExHeader, ExeFS and all four ExeFS section hashes: **GOOD**.
- Both original and New 3DS memory modes use the existing 64 MB configuration.

Ticket, TMD and access-descriptor Nintendo signature checks fail as expected
for this homebrew build. Do not describe it as retail-signed or treat ctrtool's
exit code alone as proof that every signature/hash passed. Content hash
verification and physical installation testing are separate checks.

The checksum identifies this exact committed binary. Repackaging can change
it because makerom generates ticket metadata; compare new builds by source,
embedded version and verified content hashes as well as recording their hash.

## Regression checks performed

All of these passed for this build:

```sh
sh tools/test_host.sh
sh tools/test_v10.sh
sh tools/test_v3.sh
sh tools/test_v4.sh
sh tools/test_native_keyboard.sh  # needs DEVKITPRO for real libctru headers
node tools/test_v10_demo.cjs
cmp tools/demo-v10.html dist/Lifeloch-v0.10-Demo.html
```

The keyboard test previously expected a 16-character stored profile name.
It now checks the existing Model behavior: eight characters, with cancellation
preserving that name. No production input/model code changed. The host keyboard
boundary is mocked; this does not exercise the console's OS keyboard applet.
The real Chromium click/slider test was not rerun for this build-only change.

## Publish a replacement build

After checking the build, copy the CIA to its versioned `dist/` filename and
update this file, `BUILD_STATUS.txt`, `RELEASE_INSTRUCTIONS.txt`, `CHANGELOG.md`
and `SHA256SUMS.txt`. Commit only the versioned CIA, not temporary ELF, SMDH,
banner or object outputs. Keep the existing demo copies byte-identical.
The `tools/package_v10.py` source/demo ZIP intentionally omits executable files;
the CIA is distributed separately from `dist/`.
