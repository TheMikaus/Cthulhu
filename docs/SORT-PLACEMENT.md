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
