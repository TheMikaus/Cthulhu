# Experimental Rosalina diagnostic

The first HOME Menu runtime is intentionally read-only. It adds a Rosalina
miscellaneous-menu command that maps HOME Menu's heap, searches for structurally
valid `Launcher.dat` and `SaveData.dat` images, and writes candidate addresses,
title-category counts, and per-candidate dump results to:

`sd:/3ds/Cthulhu/home-layout-report.txt`

Each structurally valid candidate is also saved for offline comparison as:

- `sd:/3ds/Cthulhu/candidate-Launcher-<address>.bin`
- `sd:/3ds/Cthulhu/candidate-SaveData-<address>.bin`

For the most useful single collection, first run Cthulhu from the Homebrew
Launcher and select `HOME Menu sorting options` -> `Sort A-Z`. This creates
`sd:/3ds/Cthulhu/sort-request.bin`, which supplies an independently collected
installed-title list. Then chainload the diagnostic payload, leave HOME Menu
open, and run `Rosalina menu` -> `Miscellaneous options` ->
`Scan HOME Menu layouts`. Progress is displayed on screen for every scan stage.
Copy the report, request, and all candidate files together for analysis.

It does not modify HOME Menu memory or savedata. Candidate addresses from several
console regions/builds are required before mutation support is enabled.

The accompanying Dockerfile extends the pinned devkitARM image with pinned
versions of `firmtool` and `makerom`, the additional tools needed to build Luma3DS.
Experimental firmware should only be chainloaded after keeping a known-good
official `boot.firm` and recovery path.

Run `build_luma_diagnostic.ps1` from the repository root. It checks out the
pinned Luma commit into an ignored build directory, applies the small menu patch,
copies the diagnostic source, and produces
`runtime/out/CthulhuHomeMenuDiagnostic.firm`.

The current build intentionally leaves Luma's early loader unchanged. HOME Menu
starts before Rosalina's 3GX service is ready, so attempting early injection can
prevent boot. Instead, chainload this payload normally, open Rosalina after HOME
Menu appears, and use **Miscellaneous options > Attach HOME Menu runtime**. The
attachment diagnostic maps the already-running HOME Menu read-only, validates
the exact USA title, hook signatures, and live layouts, then writes
`sd:/3ds/Cthulhu/home-runtime-attach.txt`.
