# HOME Menu runtime protocol

Cthulhu cannot open `Launcher.dat` while HOME Menu owns its system-save archive.
Sorting is therefore split into a producer and a consumer:

1. Cthulhu enumerates installed titles, reads their SMDH names, sorts them, and
   atomically writes `/3ds/Cthulhu/sort-request.bin`.
2. A runtime injected into HOME Menu validates the request and applies its title
   order to the already-loaded NAND and SD layout buffers.

## Request version 1

The binary format is defined in `source/home_menu_request.h`. All integers are
little-endian. The header contains a format version, selected algorithm, payload
length, and CRC-32. It is followed by fixed-size entries in desired display order.
Each entry includes the stable title ID, media type, and UTF-16 short title.

Keeping the title text in the request makes the same artifact suitable as the
index for a later search-and-focus overlay. Future sort algorithms change entry
order and the header's algorithm field; they do not require a new runtime API.

## Runtime safety requirements

The consumer must fail closed. It must not alter HOME Menu memory unless:

- the request magic, version, size, entry count, and CRC are valid;
- every media type and title ID maps to at most one loaded layout entry;
- every assigned position is unique and in range;
- both HOME Menu layout formats are recognized;
- the HOME Menu build matches a supported signature set.

`source/home_menu_layout.cpp` implements this transaction boundary. It builds a
list of old/new position mutations without touching either buffer, then rechecks
title IDs, slots, folder membership, old positions, new-position bounds, and
uniqueness immediately before applying the transaction.

The initial algorithm retains the exact set of positions currently occupied by
matching, unfiled icons. It assigns the requested title order to those positions.
Consequently, folders, unmatched system icons, gaps, and the gamecard launcher do
not move. Titles present in AM's installed-title list but absent from HOME Menu are
ignored, while duplicate loaded title IDs or positions abort the whole operation.

Version 1 should preserve folders: only entries whose existing folder ID is `-1`
are assigned new main-menu positions. Folder manipulation can be added as an
explicit later protocol version or request flag.

The runtime should apply ordering after `Launcher.dat` (0x2490 bytes) and
`SaveData.dat` (normally 0x2DA0 bytes) have been read and validated, but before
HOME Menu constructs the visible icon layout. HOME Menu should remain responsible
for persisting its own buffers.

## Next implementation slice

Create a minimal HOME Menu runtime with no overlay. It should capture the two
specific layout-read call sites, validate a request, map title IDs to layout slots,
and produce a dry-run report on SD. Memory mutation should remain disabled until
the report has been verified on every supported region/build.
