# LumaHome title-placement path

This note describes where a sorted title receives its HOME Menu coordinate. It
also distinguishes the live combined layout from the two persisted layouts.

## Coordinate model

HOME stores each icon position as one linear `s16`; it does not store separate
X and Y values. For a grid with `rows` rows, HOME's native coordinate advances
down a column first:

```text
native_position = column * rows + row
```

For row-major sorting, LumaHome converts a sorted rank to the desired visual
row and column, then converts that pair back to HOME's native coordinate.

`VisibleHomeColumns` selects the assumed number of visible columns.
`CompactTraversalPosition` performs the rank-to-position conversion.
`TraversalKey` performs the inverse ordering operation when existing sparse
coordinates are retained.

The current defect is upstream of title comparison: the live screenshot and
transaction can disagree about the active geometry. RC46 recorded four rows
and eight columns while `HNI_0065.JPG` visibly showed five rows and ten
columns. Therefore `CompactTraversalPosition` received incorrect geometry.
Changing only its column table cannot fix a stale row source.

## Placement stages

1. `CompareRequestEntries` and the surrounding request sort establish title
   rank by name. They do not assign HOME coordinates.
2. The per-group loop builds a mutation for every title. With Collapse ON it
   initially calls `CompactTraversalPosition` for each top-level/folder group.
3. The folder-aware Collapse path combines top-level titles and folders. It
   calls `CompactTraversalPosition` again using `titleBase` and `folderBase`;
   these calls are the authoritative live positions for the main HOME screen.
4. Those combined positions are copied into `g_liveTitlePositions` and
   `g_liveFolderPositions` before persistence is split.
5. SD titles are assigned a second set of media-local persisted positions.
   Launcher titles retain their combined positions. This split is necessary
   because HOME merges the two stores during boot.
6. The live update copies the merged `g_sortGrid` into HOME's processed model,
   copies the SD raw layout, and writes Launcher title/folder positions into
   the resident Launcher object.

## Primary functions and call sites

- `VisibleHomeColumns`: assumed visible width for a row/zoom selection.
- `TraversalKey`: interprets an existing native coordinate in visual order.
- `CompactTraversalPosition`: converts sorted rank to native coordinate.
- Combined title placement: assignment using `titleBase + i`.
- Combined folder placement: assignment using `folderBase + i`.
- SD persistence regrouping: assignment using `sdIndex++`.
- Live buffer commit: copies `g_liveRaw` and `g_sortGrid`, then updates resident
  Launcher positions.

All of these are currently in
`sysmodules/rosalina/source/menus/home_menu_diagnostics.c` in the nested
Luma3DS repository.

## Required next correction

Do not change the width table again based only on a screenshot. First locate
the active renderer's zoom/row value, or prove which resident copy is updated
when the enlarge/reduce buttons are pressed. Log both the Launcher-derived row
and renderer-derived geometry in the same transaction. Placement should proceed
only when they agree or when the renderer value has an independently validated
mapping.

## What public documentation actually exposes

3dbrew documents the persisted main-menu state in `Launcher.dat`:

- `0xB51` is a `u8` containing the number of HOME rows minus one. Its stored
  range is 0–5, representing one through six rows.
- `0xB5C` is the main-menu cursor position.
- `0xB5E` is the horizontal scroll level. Dividing it by the row count gives
  the number of hidden columns.
- `0xD9A` contains 360 linear `s16` icon positions. These are explicitly not
  X/Y pairs.
- `0x1434 + folderId` is the row count for that folder (default two).
- `0x1470 + folderId*2` and `0x14E8 + folderId*2` are the folder cursor and
  horizontal-scroll values.

HOME's resident Launcher object used by this project starts at file offset +8,
so the persisted main row byte is read at resident-object offset `0xB49`.

No public 3dbrew, Nintendo SDK, libctru, NS, or APT documentation located in
this project exposes a command for the renderer's immediately active zoom,
visible column count, icon pitch, or viewport width. Nintendo documentation
confirms six enlarge/reduce states and that the smallest icons can show 60 at
once, but does not define a machine-readable render-state interface.

An older published HOME layout reference enumerates the visual states as
1x3, 2x3, 3x5, 4x6, 5x8, and 6x10. This is useful historical UI information,
not proof that a particular live resident object is synchronized with the
currently drawn frame. RC45/RC46 screenshots and transaction logs demonstrate
that using the selected resident Launcher's row byte as that proof is unsafe.

Therefore the documented read path is sufficient for persistence, reboot
layout, cursor restoration, and scroll restoration. Reading the active render
geometry without closing/reopening HOME requires reverse-engineering a HOME
renderer/layout object or hook; it cannot be implemented solely through a
documented SDK call.
