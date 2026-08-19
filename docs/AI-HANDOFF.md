# Cthulhu HOME Menu Framework — AI Handoff

Last updated: 2026-08-08

## Purpose

This repository is being extended into a Nintendo 3DS HOME Menu hook/framework.
The immediate feature is sorting HOME Menu titles and folders from an overlay.
The longer-term goals are:

- multiple selectable sorting algorithms;
- independent folder placement (folders before or after individual titles);
- sorting titles inside folders;
- live sorting without rebooting;
- search from the HOME Menu;
- a reusable framework for additional HOME Menu plugins/hooks.

The user strongly prefers few device/SD-card round trips. Every experimental
build should collect enough diagnostics automatically to explain success or
failure. Never ask the user to transcribe values that can be written to a log.

## User-facing behavior already validated

- The overlay opens with **L+Y only**.
- It renders on the top screen.
- Input is responsive and HOME navigation is suppressed while the overlay is
  open.
- Power-off works with recent builds.
- The visible overlay version must be bumped for every deployed build.
- The sorting generator produces correct named-title order for A–Z and Z–A.
- Since V145, unnamed catalog entries remain after named entries in either
  direction.

The overlay, input ownership, display transfer, title catalog, and sort
calculation are not the current blockers. Applying the result to HOME's visible
icon model and persisting it safely are the current blockers.

## Repository and source-of-truth files

Repository root:

`C:\Users\white\OneDrive\Documents\3ds Home Tool\Cthulhu`

Canonical files (edit these, then copy them into the isolated Luma build tree):

- `runtime/luma/source/cthulhu_home_static_stub.s`
- `runtime/luma/source/cthulhu_home_static_patch.c`
- `runtime/luma/source/cthulhu_home_static_patch.h`
- `runtime/luma/source/cthulhu_runtime_logger.c`
- `runtime/luma/source/cthulhu_runtime_logger.h`
- `runtime/luma/source/home_menu_diagnostics.c`
- `runtime/luma/source/home_menu_diagnostics.h`
- `runtime/luma/rosalina-menu.patch`
- `runtime/luma/home-static-loader.patch`

Other relevant files:

- `source/main.cpp` — original Cthulhu implementation and proven archive-write
  patterns.
- `source/home_menu_layout.cpp/.h`
- `source/home_menu_request.cpp/.h`
- `build_luma_diagnostic.ps1`
- `docs/home-menu-runtime.md`
- `runtime/.analysis/HomeMenu-USA-code.bin`

`runtime/.build/Luma3DS` is a generated/build mirror, not the canonical source.
The worktree is intentionally dirty and contains user work. Do not clean,
reset, or discard unrelated files.

## Hardware and SD-card safety

The SD card has normally appeared as `I:` in the latest session, but verify the
drive letter every time.

Never overwrite the root SD payload:

`I:\boot.firm`

Its protected SHA-256 is:

`10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`

Hash it before and after every deployment. Abort if it differs.

Active experimental payloads belong in:

`I:\luma\payloads`

Archive the prior payload in:

`I:\luma\disabled\CthulhuFrameworkHistory`

Only one `CthulhuHomeOSD*.firm` should remain active. Use descriptive archive
names. The last payload actually installed was V147:

`I:\luma\payloads\CthulhuHomeOSD147.firm`

V148 is patched in canonical source but has **not** been successfully compiled
or installed as of this handoff.

## Target HOME Menu and confirmed addresses

- Region/version under test: USA HOME Menu
- Title ID: `0004003000008F02`
- Image: `runtime/.analysis/HomeMenu-USA-code.bin`
- Image size: `0x283000`
- Image base: `0x00100000`
- Text end: `0x00306000`
- Image SHA-256:
  `ED92758D400144E59A57D5FFA354ECBB672A6C5450494E841A2349F1BDED08F4`
- Code cave: `0x00305174..0x00306000` (`0xE8C` bytes)
- Shared channel: `0x003827F0`
- Frame hook callsite: `0x00101AAC`; original target `0x00102298`
- Display hook callsite: `0x001498F8`; original target `0x0014BC78`
- Input dispatcher: `0x001039E0`
- Button-query wrappers: `0x001F6BC8`, `0x001F6C14`, `0x001F6DC4`
- Native raw-to-grid rebuild: `0x0013C680`
- Native publish/activate: `0x00146D10`
- Layout event handler: `0x001BA594`
- Layout-controller initialization caller: `0x001D11A0`
- HID shared memory: `0x10000000`
- GSP shared memory: `0x10002000`
- OSD panel: `0x34332000`, 160x240 RGB8

The stable top-screen DMA/OSD path is already validated; avoid disturbing it.

## Confirmed runtime data

On the test console, HOME's SD layout was consistently discovered at:

- serialized raw layout: `0x346CA1E0`
- processed title grid: `0x346CCF90`
- Launcher model: `0x346BF208`

Global/wrapper memory near the channel contained:

- `0x003827D8 = 0x346BF200`
- `0x003827DC = 0x0800FE48`
- `0x003827E0 = 0x00000001`
- `0x003827E4 = 0x346CA1E0` (confirmed SD raw pointer)
- `0x003827E8 = 0x346CCF90` (confirmed SD grid pointer)
- `0x003827EC = 0x00000101`

The pair at `0x003827E4` is a serialized SD raw/grid subobject. The object at
`0x003827D8` is not itself an SD layout buffer.

## Sort request and policy

The reusable catalog is:

`/3ds/Cthulhu/sort-request.bin`

It is a v3 request converted from a preserved legacy catalog:

- 332 entries
- 223 named entries

Known bad empty-name catalog was archived as:

`sort-request-v133-empty-name-catalog.bin`

Algorithm values currently used:

- `1`: A–Z
- `2`: Z–A
- `6`: title ID order

The sort generator is confirmed to assign increasing HOME positions in the
requested name order. Example Z–A output placed `ZooVetPractice 3D`, `Zero
Escape`, then Zelda titles at increasing positions. Inside the test folder it
placed `Adventure Labyrinth Story`, `Adventure Bar Story`, then `3D Classics
Kid Icarus`, which is correct descending order.

Folder requirements:

- Folder placement is independent of sort direction.
- “Before” means folders precede individual titles.
- “After” means folders follow individual titles.
- Folder names themselves use the selected A–Z/Z–A direction.
- Titles inside each folder use the same direction.

Folder handling is currently not active in the overlay transaction:
`RunBackgroundSort` passes `stageFolders=false`, and reports show
`folder_mutations=0`.

## Shared command channel

Relevant fields relative to `0x003827F0`:

- `+0xCC`: rebuild request
- `+0xD0`: rebuild owner
- `+0xD4`: acknowledgement
- `+0xD8`: native call count
- `+0xDC`: native result
- `+0xE0`: publish owner
- `+0xE4`: captured layout-controller object
- `+0xE8`: captured event/source marker
- `+0xEC`: captured page/index
- `+0xF0`: layout-refresh result
- `+0xF4`: layout-refresh call count

The frame hook runs native work at a HOME frame boundary and acknowledges the
request. Rosalina pauses HOME threads while mutating buffers, unlocks HOME, then
sends the command and waits for the acknowledgement.

## Experiment history and conclusions

### V131–V138

- Established stable top-screen display, independent input, and diagnostics.
- Direct writes to raw, processed grid, and Launcher buffers were observed.
- Multi-copy scans found one exact raw layout, one processed grid, and one
  Launcher candidate; there was no hidden second exact raw copy.

### V139–V140

- Restored title-only path and paused all HOME threads around mutation.
- Direct live-buffer mutation still produced no visible icon change.
- Conclusion: this was not a simple cache flush, timing, or folder issue.

### V141–V145

- Added frame-bound native rebuild/publish request protocol.
- Calls to `0x0013C680` and `0x00146D10` executed and acknowledged with result
  zero.
- V143 incorrectly treated wrapper field zero as an SD layout and safely failed
  with internal `-80` (`0xFFFFFFB0`).
- V144 used parent wrapper rebuild/publish; no visible change.
- V145 rebuilt the confirmed SD subobject (`0x003827E4`) and published through
  the parent (`0x003827D8`); both returned success but icons did not move.
- Conclusion: raw/grid rebuild and publish do not refresh the already-created
  visible icon-object model.

### V146–V147

- Hooked layout event handler `0x001BA594` to capture a real controller object.
- No natural call occurred on the tested boot path, so `layout_object=0` and
  replay was safely skipped.
- Added an initialization-call capture at `0x001D11A0`, but it also did not run
  on the observed path; V147 still logged `layout_object=0`.
- The second same-boot sort failed with `-22` (`0xFFFFFFEA`) after using the
  known live addresses because the resident raw and processed grid no longer
  agreed under an overly strict precondition.
- V148 removes that unnecessary processed-grid precondition.

### Persistence defect discovered after V147

The previous `WriteSdSaveData` used `IFile_Write(..., FS_WRITE_FLUSH)` and
closed the file, but never committed the extdata archive. Persistence audits
proved that reboot restored the original extdata:

- current/original CRC: `FE99666E`
- expected sorted CRC varied by request
- approximately 170–172 position mismatches after reboot

The original Cthulhu code in `source/main.cpp` explicitly calls:

`FSUSER_ControlArchive(archive, ARCHIVE_ACTION_COMMIT_SAVE_DATA, ...)`

V148 changes `WriteSdSaveData` to the original Cthulhu pattern:

1. `FSUSER_OpenArchive`
2. `FSUSER_OpenFile`
3. `FSFILE_Write`
4. `FSFILE_Close`
5. `FSUSER_ControlArchive(...COMMIT_SAVE_DATA...)`
6. `FSUSER_CloseArchive`
7. reopen/read and compare the committed bytes

This is the most important next test.

## Current source state: V148

Canonical sources currently identify themselves as:

- visible/runtime version: `1.4.8` / `V148`
- marker: `0x43545318`
- sorter version: `0.8.7-committed-extdata`

V148 includes:

- committed extdata writes plus immediate read-back validation;
- same-boot fallback to the previously known raw/grid addresses;
- acceptance of a valid resident raw model even if the old processed grid is
  stale, because the transaction rebuilds and overwrites the grid;
- the V147 icon-controller hooks (currently unproductive but safe when the
  captured pointer remains zero).

The first V148 compile attempt failed because the code tested
`archive.handle`, while this libctru defines `FS_Archive` as an integer-like
handle. Canonical source was corrected to use an explicit `archiveOpened` bool.
The corrected V148 build has **not yet been run** and nothing from V148 has been
deployed.

## Exact next steps

1. Copy the canonical V148 source files into `runtime/.build/Luma3DS`:
   - diagnostics and logger into Rosalina;
   - static patch and stub into loader;
   - ensure `miscellaneous.c` says `Check HOME OSD v1.4.8`.
2. Compile using the existing Docker image.
3. Verify loader symbols and ensure `cthulhuHomeStubEnd < 0x14008000` after
   relocation (the actual cave limit corresponds to remote `0x00306000`).
   V147 ended at `0x140075F4`, leaving only 12 bytes before the stricter
   historical `0x14007600` observation, although the configured cave capacity
   is larger. Do not add more assembly without checking exact limits.
4. Copy the build to `runtime/out/CthulhuHomeOSD148.firm`.
5. Verify root `I:\boot.firm` hash before deployment.
6. Archive V147 descriptively and install only
   `I:\luma\payloads\CthulhuHomeOSD148.firm`.
7. Verify root `boot.firm` hash again.
8. Test one Z–A sort and reboot. Primary question: does the committed sorted
   extdata survive and does HOME load it visibly on reboot?
9. Test A–Z in the same boot. Confirm the prior `-22` no longer occurs.
10. Read logs from the SD automatically; do not request transcription.

If V148 commits and read-back succeeds but reboot still restores the old CRC,
HOME is overwriting the archive later during shutdown. In that case, do not
return to blind raw/grid calls. Trace HOME's serialization/save path or stage a
late shutdown write after HOME can no longer save stale icon state.

If reboot loads the sorted order but live icons still do not move, persistence
is solved and live refresh remains a separate task. Trace the construction or
ownership of visible icon objects, starting from the real callers of
`0x001BA594`:

- `0x001B9570`
- `0x001B9F54`
- `0x001B9FB8`

The sole direct caller found for `0x001B9ED4` was `0x001D11A0`. Do not assume
that path executes on every boot simply because it exists.

## Build workflow

Existing isolated build tree:

`runtime/.build/Luma3DS`

Pinned Luma commit used to create it:

`d30ac8d1c665ed2a50dc30b291f7eb6b33e9890a`

Docker image:

`cthulhu-luma-toolchain:1`

Build command (PowerShell):

```powershell
docker run --rm `
  --volume "C:\Users\white\OneDrive\Documents\3ds Home Tool\Cthulhu\runtime\.build\Luma3DS:/project" `
  --workdir /project `
  cthulhu-luma-toolchain:1 `
  make -j2
```

Symbol validation:

```powershell
docker run --rm `
  --volume "C:\Users\white\OneDrive\Documents\3ds Home Tool\Cthulhu\runtime\.build\Luma3DS:/project" `
  --workdir /project `
  cthulhu-luma-toolchain:1 `
  /opt/devkitpro/devkitARM/bin/arm-none-eabi-nm -n `
  sysmodules/loader/loader.elf
```

Do not run `build_luma_diagnostic.ps1` unchanged while the build directory
exists; it intentionally refuses an existing `runtime/.build/Luma3DS` tree.

## Logs to inspect after every device test

On the SD card under `/3ds/Cthulhu/`:

- `sort-transaction.txt`
- `sort-journal.txt`
- `native-rebuild.txt`
- `framework-live-v<version>.txt`
- `persistence-audit.txt`
- `post-sort-SaveData.dat`
- `pre-sort-SaveData.dat`
- `boot-current-SaveData.dat`

Also inspect `luma/dumps/arm11` for new crash dumps.

Important internal errors observed:

- `-22` / `0xFFFFFFEA`: live raw/processed validation mismatch
- `-32` / `0xFFFFFFE0`: exact live model discovery failure
- `-78`: native request acknowledgement timeout
- `-79`: invalid native pointer/region
- `-80` / `0xFFFFFFB0`: wrapper field was not an SD layout
- `-81`: V148 committed-data read-back mismatch

The journal currently records the unlock result after a failed transaction,
which can obscure the original error. Prefer `framework-live-v*.txt` and
`sort-transaction.txt` for the actual sort result, and improve the journal when
convenient.

## Technical cautions

- Preserve HOME's original register/stack ABI in every hook.
- Run complex HOME work at a known frame boundary, not from the Rosalina worker
  while HOME threads are paused.
- Flush both local and remote process caches after shared-memory changes.
- Validate every static patch signature before modifying the image.
- Keep power-off behavior in every test checklist.
- Never assume a native function returning zero caused a visible UI refresh.
- Do not conflate sorting correctness, live visualization, and persistence;
  they are three independently testable layers.
- Do not reintroduce `L+R+X`; HOME consumes `L+R` for camera behavior.
- Do not use Select-based activation; another HOME component consumes Select.

## Definition of the next milestone

The next milestone is reached when V148 or its successor demonstrates all of:

1. Z–A produces a committed extdata image whose read-back matches the intended
   sorted snapshot.
2. After reboot, HOME loads that sorted order instead of restoring CRC
   `FE99666E`.
3. A second same-boot A–Z request does not fail with `-32` or `-22`.
4. Power-off remains functional.

Live icon movement without reboot remains the following milestone if committed
persistence succeeds first.

## Append-only iteration log

Starting 2026-08-08, update this document after every project iteration. Do not
rewrite or remove earlier handoff content. Append a dated entry containing:

- what changed;
- what was built or deployed, including visible version and hashes when known;
- device/log evidence and conclusions;
- failures or safety stops;
- the exact next action.

### 2026-08-08 — Handoff policy established

The user requested an append-only AI project record after every iteration.
Work resumes from canonical V148 (`1.4.8`, marker `0x43545318`, sorter
`0.8.7-committed-extdata`). V148 still needs its corrected archive-handle code
synced into the Luma build mirror, compiled, validated, and deployed. The next
test prioritizes committed extdata persistence after reboot, followed by a
second opposite-direction sort during the same boot.

### 2026-08-08 — V148 compiled and deployed

Canonical V148 was synced into the isolated Luma build after confirming that
the generated mirror lacked the final `archiveOpened` correction. The build
then completed successfully.

Deployment details:

- visible/runtime version: `V148` / `1.4.8`
- sorter version: `0.8.7-committed-extdata`
- marker: `0x43545318`
- output: `runtime/out/CthulhuHomeOSD148.firm`
- active SD payload: `I:\luma\payloads\CthulhuHomeOSD148.firm`
- payload size: 336384 bytes
- payload SHA-256:
  `4C34361E37192C111866BFB8DD898E287B51EA001306F0144E4D857AD5C10AAF`
- archived predecessor:
  `CthulhuHomeOSD147-uncommitted-extdata.firm`

Loader symbols remained inside the configured code cave:

- stub start: `0x14007024`
- frame hook: `0x1400722C`
- layout event hook: `0x140073D8`
- icon-controller initialization hook: `0x140073F8`
- display hook: `0x14007498`
- stub end: `0x140075F4`

The protected root `I:\boot.firm` hash was verified before and after deployment
and remained:

`10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`

Next action: boot V148, apply Z–A once, power off/reboot, and determine whether
the sorted extdata survives and becomes visible. Then apply A–Z during the same
boot to verify the former `-22`/`-32` repeated-sort failure is gone. Read all
results automatically from the SD logs after the card is reinserted.

### 2026-08-08 — V148 device test failed at extdata commit path

The first V148 Z–A request failed visibly with result `0xE0C046F8`. No new
crash dump was produced and the protected root payload remained unchanged.

Evidence:

- `sort-transaction.txt` reported sorter
  `0.8.7-committed-extdata`, algorithm 2, 173 calculated title mutations, and
  final result `E0C046F8`.
- `framework-live-v148.txt` reported failed state, zero applied mutations, and
  no rebuild request; therefore failure occurred before live/native work.
- `native-rebuild.txt` was stale from V147.
- persistence remained original CRC `FE99666E` with 170 position mismatches.
- no new ARM11 dump appeared.

Decoded result fields are level 28, summary 6, module 17, description 760.
3dbrew documents the exact value `0xE0C046F8` as a generic stubbed file-service
response. The original Cthulhu implementation calls
`FSUSER_ControlArchive(...COMMIT_SAVE_DATA...)` but intentionally ignores its
return value, then closes the archive. V148 instead propagated that result as a
fatal error.

Next action: create V149 with per-stage extdata logging. Tolerate only the exact
`0xE0C046F8` result from the commit-control stage, close the archive, reopen the
file, and require an exact read-back match. Any error from open/write/close or
any read-back mismatch must remain fatal. This will identify the precise stage
and test the original Cthulhu behavior in one iteration.

### 2026-08-08 — V149 staged commit diagnostics deployed

V149 implements the V148 follow-up and was compiled and deployed successfully.

Changes:

- visible/runtime version: `V149` / `1.4.9`
- sorter version: `0.8.8-verified-extdata-commit`
- marker: `0x43545319`
- `WriteSdSaveData` now records separate results for archive open, file open,
  write, file close, archive control, archive close, read-back, byte match, and
  final result in `/3ds/Cthulhu/extdata-commit.txt`.
- Only `0xE0C046F8` from the archive-control stage is tolerated, matching the
  original Cthulhu behavior. All other stage failures remain fatal.
- After archive close, V149 reopens `SaveData.dat` and requires exact equality
  with the intended sorted bytes; mismatch returns internal `-81`.

Deployment:

- active payload: `I:\luma\payloads\CthulhuHomeOSD149.firm`
- size: 336896 bytes
- SHA-256:
  `7BDBACF843BF9F79BAA2009CC8488D6BE34F17DBAD618F77CF017517F1606B99`
- V148 archived as `CthulhuHomeOSD148-fatal-control-stub.firm`.
- stub end remained `0x140075F4`.
- protected root `boot.firm` remained unchanged before and after deployment.

Next action: boot V149 and apply Z–A once. If it succeeds, reboot and inspect
visible ordering plus persistence CRC. Whether it succeeds or fails, retrieve
`extdata-commit.txt` automatically; it will identify the exact FS stage and
whether committed read-back matched.

### 2026-08-08 — V149 commit verified; HOME overwrites it later

V149 did not visibly sort, but its new diagnostics proved the filesystem path
is correct:

- archive open, file open, write, file close, and archive close all returned 0;
- exactly 11680 bytes were written;
- archive control returned the expected tolerated `0xE0C046F8` stub response;
- immediate post-close read-back returned 0 and matched every byte;
- the sort transaction calculated and applied 173 Z–A title mutations;
- raw/grid native rebuild and acknowledgement returned 0;
- no new crash dump appeared.

After reboot, `persistence-audit.txt` again showed original extdata CRC
`FE99666E` and 170 position mismatches versus the sorted snapshot. Therefore
HOME rewrites `SaveData.dat` later from stale higher-level icon state during its
shutdown lifecycle. This is no longer an archive-open, write, flush, commit, or
read-back defect.

Live icons also remained unchanged. `layout_object` remained zero, confirming
that the attempted controller capture paths still did not execute.

Next action: create V150 with an explicitly armed pending sorted snapshot.
During the existing `PTMNOTIFID_SHUTDOWN` handler, allow HOME time to perform
its stale save, then rewrite the pending snapshot, verify it by read-back, log
the shutdown stages, and disarm only after verified success. Test using a normal
power-off rather than hardware reset. Keep live icon refresh unchanged so this
iteration isolates late persistence.

### 2026-08-08 — V150 late shutdown commit deployed

V150 was compiled, validated, and deployed to isolate late persistence.

Changes:

- visible/runtime version: `V150` / `1.5.0`
- sorter version: `0.8.9-late-shutdown-commit`
- marker: `0x43545320`
- after a successful sort and committed snapshot, the transaction writes and
  validates `/3ds/Cthulhu/pending-sort-commit.bin`;
- the existing `PTMNOTIFID_SHUTDOWN` handler checks that pending snapshot,
  validates it as a HOME SD layout, waits 1.5 seconds, runs the verified
  extdata write/read-back path, and disarms the pending file only on success;
- shutdown progress/failure is recorded in `sort-journal.txt`, while detailed
  FS results continue to be written to `extdata-commit.txt`;
- live icon behavior and controller hooks were deliberately left unchanged.

Deployment:

- active payload: `I:\luma\payloads\CthulhuHomeOSD150.firm`
- size: 337408 bytes
- SHA-256:
  `BD802F0C389E8BFFEF4538CC7BD839198F640CEBD8D147E38C3FDB2A72D8FC7C`
- V149 archived as
  `CthulhuHomeOSD149-immediate-commit-overwritten.firm`;
- loader stub end remained `0x140075F4`;
- protected root `boot.firm` remained unchanged before and after deployment.

Next action: boot V150, apply Z–A, then perform a normal power-off. A roughly
1.5-second shutdown delay is intentional. Boot again and inspect whether the
order is visible. Return the SD and inspect `sort-journal.txt`,
`extdata-commit.txt`, the pending file size, and `persistence-audit.txt` to
determine whether the handler ran late enough and whether HOME overwrote again.

### 2026-08-08 — V150 sorting persistence confirmed working

The user tested V150 and reported: **“The Sort worked!”** This is the first
confirmed successful sort in the current HOME-hook implementation after the
sorting regressions.

The successful path is:

1. The overlay calculates the requested sorted `SaveData.dat` positions.
2. The immediate extdata write and exact read-back succeed.
3. HOME continues displaying its stale higher-level icon model during that
   boot.
4. A pending sorted snapshot remains armed.
5. During a normal power-off, Rosalina's shutdown handler waits 1.5 seconds,
   rewrites the sorted extdata after HOME's stale save, verifies it, and disarms
   the pending snapshot.
6. On the next boot, HOME loads and displays the sorted layout.

Conclusion: sorting calculation, extdata format, archive writes, and persistent
application are now validated. The required persistence control point is the
late shutdown commit, not the earlier raw/grid native rebuild calls.

Remaining work should be treated as separate milestones:

- verify both A–Z and Z–A through V150's late-shutdown path;
- verify a second opposite-direction request during the same boot no longer
  fails, then power off and confirm the last request wins;
- restore independent folder placement and folder-name/title sorting without
  regressing persistence;
- find the real higher-level icon-object refresh control point for live sorting
  without reboot/power-off;
- implement HOME Menu search on top of the stable overlay/input framework.

Next action: retrieve V150's SD logs when available and confirm
`shutdown-sort-committed`, a zero-result shutdown read-back, a disarmed/zero-size
pending snapshot, and a persistence audit matching the expected sorted CRC.
Then test the opposite sort direction using the same normal-power-off flow.

### 2026-08-08 — V150 opposite sort direction confirmed

The user reported that the subsequent/opposite-direction test also worked.
Both A–Z and Z–A are therefore confirmed through V150's normal-power-off,
late-shutdown persistence path.

This confirms that direction selection, named-title comparison, unnamed-title
placement, pending snapshot replacement, shutdown rewrite, and next-boot HOME
loading all work for individual titles in both directions.

Next action: during one V150 boot, apply one direction and then apply the
opposite direction before powering off normally. The second request must not
return `-22` or `-32`, and the final request must determine the order displayed
on the following boot. After that succeeds, proceed to folder placement and
folder-content sorting as a separate iteration.

### 2026-08-08 — V150 repeated same-boot sorting already confirmed

The user clarified that the two-request same-boot test had already been
performed successfully. V150 can accept one direction and then the opposite
direction before normal power-off, with the final request determining the
persisted next-boot order. The former repeated-sort `-22`/`-32` regression is
therefore considered resolved.

Next action: move to folder support. Enable the existing folder-aware planner
from the overlay, preserve independent before/after placement, sort folder
names and folder contents using the selected direction, and persist both
`SaveData.dat` and `Launcher.dat` through the proven late-shutdown mechanism.
Do not reuse the older shutdown-time live Launcher mutation that caused freezes;
stage and write a validated Launcher snapshot instead.

### 2026-08-08 — V151 folder-aware late persistence deployed

V151 enables folder handling from the HOME overlay and extends the proven late
shutdown persistence path to `Launcher.dat`.

Behavior implemented:

- visible/runtime version: `V151` / `1.5.1`
- sorter version: `0.9.0-folder-late-commit`
- marker: `0x43545321`
- `RunBackgroundSort` now calls the existing planner with `stageFolders=true`
  and passes the overlay's independent `foldersFirst` choice;
- title groups at the top level and inside folders use the selected A–Z/Z–A
  direction;
- folders are sorted by folder name using that direction;
- folder placement is independently either before or after individual titles;
- the overlay now displays `FOLDER PLACEMENT`, `BEFORE TITLES`, or
  `AFTER TITLES` rather than saying the option is not applied;
- successful folder transactions stage
  `/3ds/Cthulhu/pending-launcher-commit.bin` alongside the pending SD layout;
- normal shutdown writes and verifies `SaveData.dat`, then writes and verifies
  USA `Launcher.dat` from system savedata ID `0x0002008F`;
- Launcher FS stages and read-back are logged in
  `/3ds/Cthulhu/launcher-commit.txt`;
- both pending files are disarmed only after both verified writes succeed;
- the former broad live-memory folder copy scan and shutdown live Launcher
  mutation are removed from the active path;
- any stale legacy `pending-folder-plan.bin` is retired during shutdown.

Deployment:

- active payload: `I:\luma\payloads\CthulhuHomeOSD151.firm`
- size: 336384 bytes
- SHA-256:
  `81F3D787677729E1C4DC5F94D2572E756BB1CA8971C4E0E7F967B03040E27A2A`
- V150 archived as
  `CthulhuHomeOSD150-confirmed-title-persistence.firm`;
- loader stub end remained `0x140075F4`;
- protected root `boot.firm` remained unchanged before and after deployment.

Next action: boot V151 and test one case first: A–Z with `BEFORE TITLES`.
Perform a normal power-off, tolerate the intentional 1.5-second delay, boot
again, and verify that folders appear before individual titles, folder names
are A–Z, and titles inside the folder are A–Z. Then inspect
`sort-transaction.txt`, `extdata-commit.txt`, `launcher-commit.txt`, pending
file sizes, persistence audit, power-off behavior, and crash dumps before
testing Z–A with `AFTER TITLES`.

### 2026-08-08 — V151 folder test failed on expected dual representation

The A–Z / folders-before V151 test failed with internal `-55`
(`0xFFFFFFC9`) before Launcher staging. No new crash dump appeared and root
`boot.firm` remained unchanged.

Evidence from `sort-transaction.txt`:

- algorithm 1, 173 title mutations, one folder mutation;
- folder 0, number 1, Launcher position 188;
- an unnamed SD raw entry, title ID `0004000000384A00`, also at top-level
  position 188;
- planner logged `[TOP_LEVEL_COLLISION] position=188` and returned `-55`;
- `launcher-commit.txt` was absent because no Launcher snapshot was armed;
- both pending files were zero-sized after cleanup.

Conclusion: HOME represents the folder in both models. The Launcher folder
record and an unnamed SD raw backing entry intentionally share the same visual
position. V151 incorrectly treated that unnamed backing entry as an independent
sortable title and rejected the expected duplicate.

Next action: associate each Launcher folder with the unnamed top-level SD raw
entry at the same old position. Exclude that backing entry from the individual
title sequence and assign it the folder's new position whenever the folder
moves. Keep collision rejection for all duplicates that cannot be explained by
exactly one folder/backing-entry pair. Add the association to the transaction
log, then rebuild and retest A–Z / folders-before.

### 2026-08-08 — V152 links Launcher folders to SD backing records

Implemented the V151 collision diagnosis in the folder planner. Before forming
the top-level sortable sequence, the planner now enumerates Launcher folders
and searches top-level SD mutations at the same old position. An unnamed SD
record at that position is linked to the folder, omitted from the independent
title sequence, and assigned the exact same new position whenever that folder
moves. A second candidate or otherwise ambiguous association fails with the
new internal result `-92`; unexplained duplicate top-level positions still use
the existing `-55` collision guard.

Diagnostics:

- successful detection appends `[FOLDER_BACKING_LINK]` to
  `sort-transaction.txt`, including folder ID, mutation index, SD slot, title
  ID, and old position;
- visible/runtime version is `V152` / `1.5.2`;
- marker is `0x43545322`;
- sorter build is `0.9.1-folder-backing-link`;
- runtime log is `framework-live-v152.txt`.

Build and deployment:

- Docker build completed successfully;
- active payload: `I:\luma\payloads\CthulhuHomeOSD152.firm`;
- size: 336896 bytes;
- SHA-256:
  `B81D777B2875152B4ED0746BC3F554AE15CE90A8CC1961E90A0A02C417372580`;
- V151 archived as
  `CthulhuHomeOSD151-folder-backing-collision.firm`;
- exactly one `CthulhuHomeOSD*.firm` remains active;
- protected root `boot.firm` remained unchanged before and after deployment,
  with SHA-256
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Next action: boot V152, confirm the overlay visibly says V152, request A-Z
with folders before titles, perform a normal power-off (including the expected
1.5-second commit delay), then boot again. Verify the folder is before titles,
folder names and contained titles are A-Z, and power-off still works. On the
next SD mount inspect `sort-transaction.txt` for `FOLDER_BACKING_LINK`, both
commit logs, pending-file sizes, persistence audit, and any new crash dump.

### 2026-08-08 — V152 rejected by HOME; root cause was Launcher offsets

The V152 transaction reported success and both late commits wrote, read back,
and matched exactly. On the following boot, however, HOME displayed its
repairing/fixing-HOME-screen message. Treat V152 as unsafe. It was removed from
the active payload directory and archived as
`CthulhuHomeOSD152-home-menu-repair.firm`.

The decisive diagnosis came from checking the structure against 3dbrew rather
than extending the inferred collision model. The three Launcher folder offsets
were all eight bytes early:

- code used folder positions `0x11D4`; documented offset is `0x11DC`;
- code used folder names `0x1558`; documented offset is `0x1560`;
- code used folder numbers `0x1D50`; documented offset is `0x1D58`.

Therefore V151's apparent folder at position 188 and V152's matching unnamed
SD record were artifacts of reading adjacent Launcher fields. V152 then wrote
the wrong Launcher region, causing HOME's repair. The logs prove this was a
semantic/model error, not a failed filesystem write: `result=0`, both readback
matches were 1, and both pending files were disarmed.

### 2026-08-08 — V153 corrects Launcher folder offsets

Implemented the documented offsets above and removed the entire false
folder-backing association path. Folder records and top-level SD titles are
again independent sortable items, and the existing collision guard remains in
place for actual duplicate positions.

Version and deployment:

- visible/runtime version: `V153` / `1.5.3`;
- marker: `0x43545323`;
- sorter: `0.9.2-correct-launcher-offsets`;
- runtime log: `framework-live-v153.txt`;
- active payload: `I:\luma\payloads\CthulhuHomeOSD153.firm`;
- size: 336384 bytes;
- SHA-256:
  `66C5E3956A46E2EDB72FB97D23956EF3FF304CF28C418DAEA72FC3615DB7B57C`;
- exactly one Cthulhu OSD payload is active;
- protected root `boot.firm` remained unchanged before and after deployment.

The console's own repair may already have changed the user's folder/layout.
Do not assume it preserved the prior folder. Next action: first boot V153 and
inspect the HOME screen without invoking sort. Confirm normal boot, current
folder existence/content, visible V153, navigation, and power-off. Only after
that baseline is known should A-Z / folders-before be requested once and the
late-commit result tested.

### 2026-08-08 — Post-repair V153 baseline

The console booted normally with V153. Navigation and the other tested HOME
behavior appeared normal, but HOME's V152-triggered repair destroyed the test
folder. This confirms the damage was localized to invalid folder metadata;
the folder itself and its grouping were not recoverable through HOME's repair.

No sorting was requested during this V153 baseline boot. V153 remains active
with the corrected documented Launcher offsets.

Next action: recreate one disposable test folder through the normal HOME Menu
UI and place at least three easily identifiable titles in it, preferably with
names deliberately out of alphabetical order. Before invoking V153 sorting,
power off and boot once normally to prove the newly created folder persists on
untouched Nintendo-generated data. Then request A-Z / folders-before once,
power off normally, reboot, and inspect both folder placement and contained
title ordering. Read all V153 transaction/commit logs immediately afterward.

### 2026-08-08 — Recreated-folder baseline passed

The user recreated the test folder through HOME Menu and confirmed that it
survived a normal power-off and reboot without invoking Cthulhu sorting. This
establishes a healthy Nintendo-generated Launcher/SaveData baseline under V153.

Next action: perform exactly one V153 request using A-Z and folders-before.
After the request completes, power off normally, allow the intentional late
commit delay, and reboot. Check for any HOME repair message first, then verify
folder placement, folder-name direction, contained-title direction, ordinary
top-level title direction, navigation, and power-off. Mount the SD immediately
afterward so the V153 transaction, both commit logs, pending-file sizes,
persistence audit, and crash directory can be inspected before another sort.

### 2026-08-08 — V153 selected a stale Launcher candidate and damaged metadata

The V153 test again caused HOME repair-like behavior. The user observed theme
state being reset and one or two applications reverting to unopened gift
packages. Logs showed a successful transaction and exact filesystem readback,
but the critical discovery line was:

- `launcher_matches=5`;
- selected candidate `0x346BF208`;
- with corrected documented offsets, that selected candidate reported
  `folder_mutations=0` even though the recreated folder definitely existed.

Therefore the live-memory scan is finding multiple structurally valid
Launcher-sized snapshots and its nearest-address heuristic selected a stale or
secondary copy. Writing that entire candidate to `/Launcher.dat` overwrote
unrelated current HOME state. The later HOME repair accounts for the visible
theme/gift effects. Do not use V153 again. It is archived as
`CthulhuHomeOSD153-wrong-live-launcher-candidate.firm`.

### 2026-08-08 — V154 read-only Launcher candidate capture deployed

V154 converts the folder-capable sort path into an unconditional diagnostic
guard. After discovery it records and dumps every structurally valid Launcher
candidate, returns internal `-93`, and exits before mapping/mutating/staging
either SaveData or Launcher data. This is intentional: pressing Apply should
report failure, but must not create pending commits or change HOME data.

Candidate diagnostics:

- up to 16 candidate addresses are retained from discovery;
- `sort-transaction.txt` receives `[LAUNCHER_CANDIDATES]` with address,
  mapping result, and folder count using the documented offsets;
- every mapped candidate is dumped as
  `/3ds/Cthulhu/candidate-Launcher-<address>.bin`;
- journal stage is `ambiguous-launcher-candidates-no-write`.

Version and deployment:

- visible/runtime version: `V154` / `1.5.4`;
- marker: `0x43545324`;
- sorter: `0.9.3-launcher-candidate-guard`;
- active payload: `I:\luma\payloads\CthulhuHomeOSD154.firm`;
- size: 335360 bytes;
- SHA-256:
  `E1583F9C3DEA3E4DF1F1734F00B3F2A4C7243C34D5AEB74C851C7B999C6CB68D`;
- root `boot.firm` remained unchanged.

Next action: first allow HOME to finish its own repair and recreate a disposable
folder if necessary. Boot V154, confirm the visible version, invoke A-Z /
folders-before once, and expect an intentional failure (`-93` / `0xFFFFFFA3`).
Do not expect any sorting. Power off and reboot only to verify HOME is unchanged,
then mount the SD. Compare all dumped Launcher candidates to identify which one
contains the real folder/name/number state and determine a stable discriminator
or owner pointer before re-enabling any Launcher write.

### 2026-08-08 — V154 remained read-only but failed before candidate capture

The V154 attempt did not stage or commit anything: both pending files remained
zero bytes, commit logs were still the older V153 logs, and no new crash dump
appeared. HOME therefore received no V154 layout write. The overlay recorded
`sort_result=0xFFFFFFE0` (internal `-32`), meaning broad SD runtime discovery
failed before Launcher discovery/capture. `sort-transaction.txt` consequently
remained the older V153 report. Existing candidate dumps were from an earlier
date and predated the recreated folder, so their zero-folder results were not
usable for current-owner selection.

The address history exposed two independent layout relocations. The current SD
raw/grid pair repeatedly observed by diagnostics is
`0x346CA1E0 / 0x346CCF90`, while the actual known NAND Launcher raw address is
independently `0x3469F208`. V153 incorrectly derived a Launcher preference from
the SD relocation and preferred `0x346BF208`. NAND and SD objects do not move as
one fixed-delta pair.

### 2026-08-08 — V155 fixed-address read-only candidate dump deployed

V155 updates the known SD raw/grid constants to the repeatedly confirmed
current addresses, permits them as a validated fallback when broad scanning is
ambiguous on a fresh boot, and directly prefers `0x3469F208` for Launcher
discovery. It retains V154's unconditional `-93` guard after dumping candidates,
so this build still cannot stage or commit a sort.

Version and deployment:

- visible/runtime version: `V155` / `1.5.5`;
- marker: `0x43545325`;
- sorter: `0.9.4-fixed-address-candidate-dump`;
- active payload: `I:\luma\payloads\CthulhuHomeOSD155.firm`;
- size: 335872 bytes;
- SHA-256:
  `943281F67BA37D5F4B6E14DA011F8F0DA80B448B308E42E43A0A9F9E98CCE0AB`;
- protected root `boot.firm` remained unchanged.

Next action: boot V155, confirm the visible version, invoke A-Z /
folders-before once, and expect intentional failure `0xFFFFFFA3`. Verify no
visible HOME change, then mount the SD without another attempt. Inspect the new
`[LAUNCHER_CANDIDATES]` report and freshly timestamped candidate binaries to
find the candidate containing the recreated folder's valid name, number, and
position. Do not enable Launcher writes until that discriminator is proven.

### 2026-08-09 — V155 proved shifted live Launcher representation

V155 behaved safely and returned the intentional `0xFFFFFFA3`. Both pending
commit files remained zero, the extdata/Launcher commit logs remained V153-era,
and root `boot.firm` was unchanged.

Fresh candidate parsing produced the decisive result. Candidate `0x346BF208`
contained no folder when interpreted as a complete file, but contained exactly
one credible folder when all documented folder offsets were shifted by `-8`:

- folder ID 0;
- position 193;
- number 1;
- name `(New Folder)` (with a non-ASCII/control prefix rendered as `?` by the
  simple diagnostic parser).

All other freshly dumped candidates had zero credible folders under the shifted
interpretation. This proves the resident object at `0x346BF208` begins at
Launcher.dat file offset `+8`; it is not a complete file image. Earlier builds
copied that object as if byte 0 were the file header, shifting every persisted
field and overwriting unrelated state.

### 2026-08-09 — V156 minimal Launcher patch persistence deployed

V156 removes full live-snapshot persistence from the active folder path:

- discovery scores candidates by credible folder records using the live
  `file-offset-minus-8` representation, then uses distance only as a tie-break;
- the chosen live object is aligned into a temporary file-layout buffer solely
  for planning (`live byte 0` becomes temporary file byte 8);
- Apply stages `pending-folder-plan.bin`, containing only folder IDs, numbers,
  old positions, and new positions;
- no full pending Launcher snapshot is armed;
- after the normal 1.5-second shutdown delay and HOME's own save, the handler
  reads the real current `/Launcher.dat` from NAND system savedata;
- it saves that exact current file as
  `/3ds/Cthulhu/pre-folder-commit-Launcher.dat`;
- before each patch it requires the current folder number and old position to
  match the staged plan, otherwise it aborts with internal `-97`;
- it modifies only the corresponding two-byte position field at documented
  file offset `0x11DC + folder_id*2`, writes the current file back, and verifies
  exact readback;
- stale `pending-launcher-commit.bin` is disarmed and never consumed.

Version and deployment:

- visible/runtime version: `V156` / `1.5.6`;
- marker: `0x43545326`;
- sorter: `0.9.5-minimal-launcher-patch`;
- active payload: `I:\luma\payloads\CthulhuHomeOSD156.firm`;
- size: 337920 bytes;
- SHA-256:
  `27760F45DB9D67DBCC275A7C4D522DEF10F007D444CB8A76CE4B34E6A362D2D9`;
- V155 archived as
  `CthulhuHomeOSD155-confirmed-shifted-live-launcher.firm`;
- protected root `boot.firm` remained unchanged.

Next action: make this a single controlled A-Z / folders-before test. Confirm
V156, apply once, power off normally, and reboot. First check for any repair
message or unrelated theme/gift-state change. Then check folder survival and
placement plus inner/top-level ordering. Mount the SD immediately afterward.
Inspect the transaction's candidate/folder sections, folder plan, real-current
Launcher backup, both commit logs, journal, pending sizes, audit, and dumps.

### 2026-08-09 — V156 user-visible result identifies three remaining paths

The V156 test did not destroy the folder or trigger the previously described
full HOME repair, but it was not acceptable:

- Notifications became marked unread after the transaction/reboot;
- the folder did not move to the requested folders-before location;
- inside the folder, Adventure Bar Story was first, but Yoshi's New Island
  remained before Virtual NES under A-Z. This cannot be explained by case
  folding because `V` sorts before `Y` in either case;
- launching Notifications and returning to HOME appeared to disconnect the OSD
  hooks.

Interpretation pending log inspection:

- even reading the current Launcher and rewriting all 0x2490 bytes is too broad
  or races another HOME-owned update. The next persistence revision should
  issue only targeted 2-byte writes at each verified folder-position offset,
  with before/after readback, rather than rewriting the file image;
- inspect the V156 journal and staged folder plan to determine whether the
  folder move aborted with `-97`, selected an unchanged destination, or was
  overwritten later;
- inspect mutation logs for the three folder members to determine why their
  expected `Adventure Bar Story, Virtual NES, Yoshi's New Island` order was not
  persisted;
- treat HOME/app-transition hook lifetime as a separate problem: detect a new
  HOME PID/process image and reinstall/revalidate hooks automatically.

The user proposed a controlled folder-position experiment: place the folder at
a known location, log it, manually move it, then trigger/log A-Z. This is useful
after V156 logs are collected and after full-file Launcher writes are disabled.
The experiment should automatically record both live shifted metadata and real
file metadata before/after, avoiding manual transcription.

### 2026-08-09 — V156 logs: folder write aborted; unnamed VirtuaNES sorted last

V156 transaction planning itself was correct for folder placement:

- authoritative live candidate `0x346BF208` had one folder;
- folder 0, number 1, planned old position 193 to new position 14;
- candidate selection and shifted alignment were correct.

Shutdown failed with `0xFFFFFFA1` (internal `-95`) while validating the real
Launcher file, before `pre-folder-commit-Launcher.dat` was created and before
any Launcher write. `launcher-commit.txt` remained the older V153 report. This
explains why the folder did not move. The Notifications unread observation was
therefore not caused by a V156 Launcher write.

The title log also explains the inner-folder order. Title ID
`0004000000384A00` (VirtuaNES) had an empty catalog name. The comparator keeps
unknown names last, so V156 planned Adventure Bar Story at 0, Yoshi's New
Island at 1, and unnamed VirtuaNES at 2. This was not a capitalization issue.

Important failure-handling defect: V156 successfully rewrote SaveData, then
the later Launcher validation failed, leaving `pending-sort-commit.bin` armed
at 11680 bytes. That would have repeated the SD write at every shutdown.
Before further work, the failed sort and folder plans were copied to
`failed-v156-pending-*.bin`, all three pending files were truncated to zero,
V156 was archived, V150 was temporarily restored, and root `boot.firm` was
verified unchanged.

### 2026-08-09 — V157 targeted two-byte folder write deployed

V157 removes whole-file Launcher writes from the active folder commit path:

- the current Launcher file is still read and backed up for recovery, but no
  strict whole-layout validator blocks targeted work;
- the commit opens `/Launcher.dat` read/write and, for each planned folder,
  reads only its two-byte position and four-byte folder number;
- it requires both to match the staged old position/number;
- it writes exactly two bytes at `0x11DC + folder_id*2`;
- it immediately reads those two bytes back and verifies them;
- `launcher-commit.txt` reports planned, written, and verified counts;
- all pending files are disarmed after one shutdown attempt even if a later
  step fails, preventing repeated writes.

V157 also injects a known-name fallback `VirtuaNES` for title ID
`0004000000384A00` before sorting. Longer term this should become a generic
user-configurable alias facility.

Version and deployment:

- visible/runtime version: `V157` / `1.5.7`;
- marker: `0x43545327`;
- sorter: `0.9.6-targeted-folder-write`;
- active payload: `I:\luma\payloads\CthulhuHomeOSD157.firm`;
- size: 337920 bytes;
- SHA-256:
  `696CB1A9B4EB0C274581E7C0D8BB80348CEC25957D95653AA4876FC09C44E9C5`;
- all pending files were zero before deployment;
- protected root `boot.firm` remained unchanged.

Next action: run one A-Z / folders-before V157 test, power off, reboot, and
verify notification state first, folder movement second, then inner-folder
order (expected Adventure Bar Story, VirtuaNES, Yoshi's New Island). Mount the
SD immediately afterward. Separately, reproduce Notifications app transition
only after persistence is verified; hook reinstallation across HOME lifecycle
remains unimplemented.

### 2026-08-09 — V157 safely rejected valid cross-media position overlap

V157 returned `0xFFFFFFC9` (internal `-55`) during planning, before any
SaveData, folder-plan, or Launcher commit. All pending sort/Launcher files were
zero; `pending-folder-plan.bin` contained only the previously disarmed four-byte
magic field. The persistence audit matched the expected V156 snapshot exactly.

The collision report identified position 193. The live Launcher folder record
reported folder 0 at position 193, while SD title ZooVetPractice 3D also
reported top-level position 193. HOME accepts and displays this baseline, so
NAND/Launcher folder coordinates and SD title coordinates are not one globally
unique position namespace. The planner's combined duplicate rejection is
invalid. It must model HOME's merge/precedence behavior instead of treating an
equal numeric position across media as corruption.

The V157 title plan confirmed the VirtuaNES alias fix works: inside folder 0 it
planned Adventure Bar Story `2 -> 0`, VirtuaNES `2 -> 1`, and Yoshi's New
Island `1 -> 2` (the duplicate old value shown across two records is further
evidence that only target uniqueness, not all source cross-model uniqueness,
can be required).

### 2026-08-09 — V158 read-only folder-coordinate observer deployed

V158 restores an unconditional diagnostic guard immediately after Launcher
candidate capture. Any Apply request returns `-93` / `0xFFFFFFA3` before
mapping, planning, staging, or writing data. It is intended solely for the
user's controlled manual-movement experiment.

Version and deployment:

- visible/runtime version: `V158` / `1.5.8`;
- marker: `0x43545328`;
- sorter: `0.9.7-folder-coordinate-observer`;
- active payload: `I:\luma\payloads\CthulhuHomeOSD158.firm`;
- size: 336384 bytes;
- SHA-256:
  `685DF85B133679CDED4B54ADF79EE9D0304CC373476DF509684A39F3BD8B9225`;
- V157 archived as
  `CthulhuHomeOSD157-expected-cross-media-collision.firm`;
- protected root `boot.firm` remained unchanged.

Known pre-movement observation: live folder 0, number 1, position 193. Next
action: with V158 active, manually move that folder through HOME to a clearly
different visible location, allow HOME to settle/save, then invoke Apply once.
Expect `0xFFFFFFA3` and no sort. Mount the SD. Parse the newly dumped shifted
candidate to obtain the post-movement folder coordinate and compare SD title
positions around both visible locations. Use that delta to infer HOME's
cross-media merge rule before re-enabling targeted folder writes.

### 2026-08-09 — V158 live snapshot stayed at 193 after manual movement

V158 returned the intentional `0xFFFFFFA3`, staged no sort data, and left all
layout commit files disarmed. The authoritative shifted candidate still
reported folder 0, number 1, position 193 after the user manually moved the
folder to a different visible location. Therefore that resident Launcher
snapshot is either stale for drag operations or is not synchronized until
HOME's shutdown save. It cannot by itself establish the visual merge mapping.

### 2026-08-09 — V159 read-only shutdown Launcher observer deployed

V159 remains read-only. Apply still dumps candidates and intentionally returns
`0xFFFFFFA3`, but it now stages a special zero-folder observation plan with
algorithm `0xFFFF`. On the following shutdown, independently of any pending
sort, the notification handler:

- waits the normal 1.5 seconds for HOME's own save;
- reads the real current NAND system-savedata `/Launcher.dat`;
- copies it unchanged to
  `/3ds/Cthulhu/pre-folder-commit-Launcher.dat`;
- disarms the observation plan;
- records `shutdown-observation-captured` or
  `shutdown-observation-failed` in the journal.

It performs no SaveData write and no Launcher write. This directly determines
whether the manually moved position reaches the persisted file even though the
live snapshot stayed at 193.

Version and deployment:

- visible/runtime version: `V159` / `1.5.9`;
- marker: `0x43545329`;
- sorter: `0.9.8-shutdown-launcher-observer`;
- active payload: `I:\luma\payloads\CthulhuHomeOSD159.firm`;
- size: 336384 bytes;
- SHA-256:
  `98CEFF1591448772C29D3434F04ED1AB0053D60F1DDFC65B1F9F393222677344`;
- protected root `boot.firm` remained unchanged.

Next action: boot V159 without moving the folder again, invoke Apply once,
expect `0xFFFFFFA3`, then power off normally and mount the SD. Parse folder 0
from the captured real Launcher file at documented offsets. Compare its saved
position to live 193 and to the user's visible manual destination. Do not
enable folder writes until this persisted observation is understood.

### 2026-08-09 — V159 captured authoritative folder position 16

The shutdown observation completed successfully (`result=0`) and copied the
real current Launcher file unchanged. Parsing documented file offsets found:

- folder 0;
- position 16;
- number 1;
- name `(New Folder)`.

The resident candidate in the same boot still reported position 193. Thus the
live candidate is usable for folder identity/name/number discovery but not for
current position after user drag/rearrangement. The real system-savedata file
read after HOME's shutdown save is authoritative for position.

### 2026-08-09 — V160 isolated folder-position-only test deployed

V160 keeps normal title sorting disabled. Apply still returns the intentional
`0xFFFFFFA3`, but stages a special algorithm `0xFFFE` folder-only plan:

- it reads folder IDs/numbers from the credible live candidate;
- it computes the first current top-level SD-title coordinate from SaveData;
- it stages each folder target starting at that coordinate;
- old position is deliberately `-1`, because only the shutdown-read real file
  has the authoritative old position;
- no pending SaveData commit is created.

At shutdown V160 waits 1.5 seconds, reads and backs up the real current
Launcher, verifies each folder number, writes only its two-byte position field,
immediately reads it back, commits, disarms the plan, and logs the result. This
tests whether a folder tied at the first SD coordinate renders before the SD
title under HOME's cross-media merge precedence, without changing any SD icon
positions.

Version and deployment:

- visible/runtime version: `V160` / `1.6.0`;
- marker: `0x43545330`;
- sorter: `0.9.9-folder-position-only`;
- active payload: `I:\luma\payloads\CthulhuHomeOSD160.firm`;
- size: 336896 bytes;
- SHA-256:
  `CD85AD466F7B2513256700D68C354CF05BD86F0C5C287E5E40E2C47F7030CBF3`;
- root `boot.firm` remained unchanged.

Next action: boot V160 without manually moving the folder, Apply A-Z /
folders-before once, expect `0xFFFFFFA3`, then power off and reboot. Verify no
notification/theme/gift changes and report whether the folder moves relative
to the first SD title. Mount the SD. Inspect the targeted commit counts/result,
backup old position (expected 16), and post-boot real position before restoring
combined title sorting.

### 2026-08-09 — V160 targeted mechanism confirmed safe

The user reported that the folder placement appeared to work. Logs confirmed
the targeted persistence path itself was fully successful:

- planned folders: 1;
- two-byte writes: 1;
- immediate readback verifications: 1;
- archive commit and close: success;
- overall result: 0;
- no SaveData commit was staged;
- protected root remained unchanged.

The pre-write real Launcher backup already contained folder position 14, so
this specific write was a verified 14-to-14 no-op. This likely reflects HOME
normalizing the earlier manual position on the intervening reboot. Crucially,
position 14 rendered the folder before the first SD title even though an SD
title can share the numeric coordinate. This establishes the practical merge
rule needed for folders-before: target the first SD-title coordinate and do not
insert the folder into or shift the SD position set.

### 2026-08-09 — V161 integrated independent folder placement deployed

V161 removes the cross-media combined-position planner and restores normal
title sorting. SD titles continue using the proven per-group sort that preserves
each group's existing position set. Folder placement is calculated separately:

- folders-before: sorted folders target `first_sd_position + index`;
- folders-after: sorted folders target `last_sd_position + 1 + index`;
- folder names follow the selected A-Z/Z-A direction;
- SD title positions are not shifted to make room for folders;
- live folder positions are recorded as `-1` because they are stale; shutdown
  verifies folder identity/number against the real file and performs targeted
  two-byte writes;
- VirtuaNES keeps its explicit name fallback, so the expected folder order is
  Adventure Bar Story, VirtuaNES, Yoshi's New Island.

Version and deployment:

- visible/runtime version: `V161` / `1.6.1`;
- marker: `0x43545331`;
- sorter: `1.0.0-independent-folder-placement`;
- active payload: `I:\luma\payloads\CthulhuHomeOSD161.firm`;
- size: 337920 bytes;
- SHA-256:
  `884F6712F8CA168C3B0C56B152E16297C6C6E99ABFC5B614CC39EAA2B334B5E0`;
- V160 archived as
  `CthulhuHomeOSD160-confirmed-targeted-position-write.firm`;
- protected root `boot.firm` remained unchanged.

Next action: perform one integrated V161 A-Z / folders-before request, power
off, and reboot. Verify unrelated state first, then top-level title order,
folder placement, and inner-folder order. Mount the SD immediately afterward
to inspect title persistence, targeted folder commit, and pending cleanup. Do
not test Z-A/folders-after until this combined path passes.

### 2026-08-09 — V161 integrated A-Z / folders-before passed

The user reported the combined result appeared correct, and all diagnostics
confirmed success:

- transaction result: 0;
- shutdown journal: `shutdown-sort-committed`, result 0;
- title mutations: 173;
- folder mutations: 1;
- folder 0 targeted position 14;
- SaveData write/readback matched;
- late persistence audit matched exactly with zero byte and zero position
  mismatches;
- targeted Launcher commit planned/wrote/verified exactly one two-byte field;
- folder contents were planned as Adventure Bar Story at 0, VirtuaNES at 1,
  and Yoshi's New Island at 2;
- `pending-sort-commit.bin` and `pending-launcher-commit.bin` are zero bytes;
- `pending-folder-plan.bin` retains its fixed struct size but has a cleared
  magic field, so it is disarmed;
- no new crash dump;
- protected root `boot.firm` unchanged.

V161 is the first confirmed integrated milestone for persistent top-level A-Z,
independent folders-before placement, folder-member A-Z sorting, and targeted
Launcher persistence without HOME repair or broad Launcher rewrites.

Next action: test the opposite preset on the same build: Z-A with folders-after.
Expected behavior is top-level SD titles Z-A, folder members Yoshi's New Island,
VirtuaNES, Adventure Bar Story, and the folder after individual SD titles.
Power off/reboot once and inspect the same logs before declaring both directions
stable. After that, return to hook lifecycle recovery across Notifications and
other HOME restarts, then live refresh/search work.

### 2026-08-09 — V161 Z-A / folders-after passed

The user confirmed the opposite preset visually. Logs independently verified:

- algorithm 2 transaction result 0;
- 173 title mutations and one folder mutation;
- folder 0 independently targeted position 202 (after SD titles);
- shutdown sort commit result 0;
- targeted folder commit planned/wrote/verified one field;
- pending sort and Launcher snapshot files are zero;
- fixed-size folder plan has cleared magic and is disarmed.

V161 now has confirmed persistent behavior in both directions.

### Requested feature backlog after V161 milestone

User-requested remaining features:

1. Optional hole collapse: compact occupied icons into contiguous positions.
2. Traversal orientation: row-major (left-to-right, top-to-bottom) or
   column-major (top-to-bottom, left-to-right).
3. Folder bands: place folders in top or bottom rows rather than only before or
   after titles in the linear sequence.
4. Managed alphabetical folders: create only needed A-Z folders, place titles
   into them, mark Cthulhu-created folders distinctly (user suggested an
   underscore prefix), and when disabled move contents out and remove only
   Cthulhu-owned folders.
5. Apply sorting without reboot.
6. Filter icons live for quick HOME search.

Important design notes:

- managed folders must have an SD-side ownership registry keyed by folder
  number/ID and expected name; never delete a folder solely because its visible
  name begins with `_`;
- managed-folder disable must be transactional, previewable, and refuse removal
  if identity/name/contents no longer match the managed record;
- row/column traversal and folder bands require a shared 2D layout mapper aware
  of HOME's selected row count, reserved system positions, and bounds;
- hole collapse should be an independent option consumed by that mapper;
- live sort and filtering both require solving hook reinstallation across HOME
  process/reload transitions (Notifications currently demonstrates hook loss)
  and obtaining a reliable live layout rebuild/publish call;
- filtering should initially be visual/non-destructive and restore the exact
  prior grid when the search overlay closes.

Recommended implementation order:

1. hook lifecycle monitor/reinstall after HOME restarts;
2. pure layout engine with collapse-holes and row/column traversal, backed by
   host-side tests;
3. folder top/bottom row bands using the same mapper;
4. managed A-Z folder ownership/preview/undo transaction;
5. live rebuild for no-reboot sorting;
6. reversible live icon filtering and quick search.

Additional desirable features: dry-run preview and change counts, one-action
undo/restore from verified backups, pinned/excluded titles, user title aliases,
saved sorting profiles, additional keys (title ID, last played, play count,
manual priority), multi-region/address profiles, and an on-device diagnostics
export/status page.

### 2026-08-09 — V162 hook lifecycle recovery diagnostic built

Started the first post-V161 backlog item. Inspection showed that the loader
already invokes `CthulhuHomeStaticPatch_Apply` whenever the USA HOME Menu title
is loaded, so Notifications-related hook loss cannot safely be assumed to be
only a missing patch on a replacement process.

V162 adds conservative lifecycle observation and channel recovery in Rosalina:

- visible/runtime version `V162` / `1.6.2`;
- marker `0x43545332` and framework diagnostic version
  `0.7.2-lifecycle`;
- logs HOME PID transitions, marker, heartbeat, panel address, the frame-hook
  callsite word, the code-cave stub word, and recovery count to
  `/3ds/Cthulhu/hook-lifecycle-v162.txt`;
- distinguishes a new process with a valid static hook, an invalid static hook,
  and a valid hook waiting for its shared channel;
- if the marker is lost while the linked V162 frame callsite, `E92D500F` stub,
  and allocated panel are all still intact, republishes only the marker and ABI;
  it does not rewrite executable HOME code or allocate a replacement panel;
- the normal periodic state log is now
  `/3ds/Cthulhu/framework-live-v162.txt`.

The V162 linker map places `cthulhuFrameHook` at stub offset `0x208`, producing
runtime target `0x0030537C`; lifecycle validation uses that linked address.
Docker build succeeded. Built payload size is 338432 bytes with SHA-256
`216E3C5C51FCA8420FE9E1B9F745977DBFE92CF74BFE91F9797036283F9F7A4E`.

Next test: boot V162, confirm the overlay visibly says V162 and opens with L+Y,
open Notifications, return to HOME, then test L+Y again and power off. Inspect
both V162 logs afterward. If recovery fails, the lifecycle log should identify
PID replacement, static-hook mutation, missing panel state, or channel loss
without requiring the separate diagnostic application.

### 2026-08-09 — V162 Notifications test failed; V163 shared input built

The user reported that the overlay did not work after the Notifications test.
The SD was already mounted and the automatic V162 logs were inspected. They
showed:

- HOME remained PID 15;
- marker `43545332`, frame hook `EB080E32`, expected frame hook `EB080E32`,
  and stub `E92D500F` were all valid;
- the lifecycle log contained only the initial valid attachment;
- the periodic runtime log stopped after heartbeat 659 even though it normally
  rewrites every two seconds from Rosalina.

This disproves the initial replacement-process/static-hook-loss hypothesis for
this run. The evidence instead points to the runtime thread blocking in
`hidScanInput()` during the system-application transition. Because execution
stopped inside that call, the lifecycle monitor itself could not continue and
could not record a later state.

V163 removes all `hidInit`/`hidScanInput`/`hidKeysHeld` use from the Cthulhu
runtime thread. It consumes the held-key word already published by HOME's
static frame hook at channel offset `+0x10`. The HOME hook now publishes zero
values as well as nonzero values for both held (`+0x10`) and down (`+0x14`), so
release transitions cannot leave stale keys. Existing pressed-key behavior is
derived locally from held-state edges.

V163 identifiers and artifacts:

- visible/runtime `V163` / `1.6.3`;
- marker `0x43545333`;
- framework diagnostic `0.7.3-shared-input`;
- logs `hook-lifecycle-v163.txt` and `framework-live-v163.txt`;
- linked frame-hook offset remains `0x208`, target `0x0030537C`;
- build succeeded, size 338432 bytes, SHA-256
  `5DD16FA76FF51EAE67947EE4F1D5AE7200FC64A004AB0E4AF5CD135689EEBD63`.

Next test is the same short lifecycle test: confirm V163 and normal L+Y, close,
open Notifications, return HOME, retry L+Y, and verify power off. The runtime
should now continue logging even across the transition without an HID service
call that can block.

### 2026-08-09 — V163 overlay regression; V164 cache-coherent input built

The V163 overlay did not open even before the Notifications transition. Its
automatic logs proved the static system remained healthy: PID 15, marker
`43545333`, frame callsite `EB080E32`, stub `E92D500F`, heartbeat 703,
display hooks 2107, and no crash. However, Rosalina always observed held input
as zero and `render_count` stayed zero.

The V163 shared-input address was correct (`channel +0x10`, sourced from the
current HOME HID ring entry), but the two processes have separate ARM data
caches for the mapped channel. The former `hidScanInput()` path refreshed HID
implicitly; direct mapped reads did not establish cache coherency at each
sample. V164 flushes HOME's mapped channel page and Rosalina's local mapping
before reading held input each frame. It also records `shared_held`,
`last_nonzero_held`, and `input_transitions` in the periodic log so an input
failure is diagnosable even after the keys are released.

V164 identifiers and artifacts:

- visible/runtime `V164` / `1.6.4`;
- marker `0x43545334`;
- framework diagnostic `0.7.4-cache-input`;
- logs `hook-lifecycle-v164.txt` and `framework-live-v164.txt`;
- linked frame-hook offset remains `0x208`, target `0x0030537C`;
- build succeeded, size 338432 bytes, SHA-256
  `63D36D7D614DBEAE520138E7B9C21EAF049B28C04759128310577EAC4494C927`.

Next test should first verify that L+Y opens V164 normally. Only if that passes,
continue through Notifications and retry L+Y. If normal activation still fails,
inspect the three new input fields before changing the transport again.

### 2026-08-09 — V164 failed; active payload rolled back to V162

The user reported that V164's overlay did not work at all and correctly asked
why the hook was changed. Clarification: the installed static hook addresses
were not changed, but V163/V164 changed the overlay's input transport from the
known-working Rosalina `hidScanInput()` path to a channel word published by the
HOME frame hook. That replacement was based on an unproven inference from the
V162 log stopping after Notifications. It regressed initial overlay activation
in both V163 and V164 and should not have replaced a proven subsystem before an
independent watchdog isolated the failure.

The SD was mounted. `CthulhuHomeOSD164.firm` was moved to the history directory
and the exact archived V162 payload was restored as the sole active payload at
`/luma/payloads/CthulhuHomeOSD162.firm`. Its SHA-256 is the previously verified
`216E3C5C51FCA8420FE9E1B9F745977DBFE92CF74BFE91F9797036283F9F7A4E`.
The protected root `boot.firm` remained unchanged with SHA-256
`10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Next implementation must branch from V162 behavior, retain its HID input and
all working sorting behavior unchanged, and add a separate watchdog thread for
PID/hook/heartbeat diagnostics. The watchdog must not call HID and must write
its own log even if the overlay runtime thread blocks. Do not resume the
shared-input experiment unless diagnostics independently prove it necessary.

### 2026-08-09 — V165 independent watchdog built from V162 behavior

V165 restores the proven V162 overlay input path exactly: Rosalina initializes
HID, calls `hidScanInput()`, and uses `hidKeysHeld()`. The HOME stub's HID ring
publishing logic was also restored to its V162 conditional form. No shared-word
input experiment remains in the active source.

A second, independent Rosalina thread was added solely for lifecycle evidence.
It never initializes or calls HID and never changes HOME or overlay state. Once
per second it independently opens the USA HOME process, maps and reads the
channel, unmaps it, reads the frame callsite and code-cave stub through separate
temporary mappings, and writes `/3ds/Cthulhu/watchdog-v165.txt`. The log records
PID and PID-change count, marker, heartbeat and previous heartbeat, consecutive
stall samples, frame hook versus expected hook, stub word, sample count, and a
classified state (`healthy`, `heartbeat-stalled`, `marker-invalid`,
`hook-invalid`, or `read-error`). Because it is a separate thread with no HID
call, it should continue writing if the V162-style overlay thread blocks.

V165 identifiers and artifacts:

- visible/runtime `V165` / `1.6.5`;
- marker `0x43545335`;
- framework diagnostic `0.7.5-watchdog`;
- regular logs `hook-lifecycle-v165.txt` and `framework-live-v165.txt`;
- independent log `watchdog-v165.txt`;
- linked frame-hook offset remains `0x208`, target `0x0030537C`;
- build succeeded after adding a required `openHome` forward declaration;
- payload size 338944 bytes, SHA-256
  `0D13EB1003AC328B10225B9804FBD00A455CE1708934636868035DFEC16CAAAB`.

Next test: confirm V165 and initial L+Y behavior first. Then close the overlay,
open Notifications, return HOME, and retry L+Y. Power off and inspect all three
V165 logs. The watchdog log is the authoritative lifecycle record if the
regular runtime log stops.

### 2026-08-09 — V165 isolated Notifications failure; V166 direct HID built

The user confirmed initial V165 L+Y activation worked and post-Notifications
activation did not. The independent watchdog continued running and proved:

- HOME remained PID 15 with zero PID changes;
- frame callsite `EB080E32`, expected callsite `EB080E32`, and stub
  `E92D500F` remained intact;
- marker remained `43545335`;
- heartbeat stopped at 1894 for at least 27 watchdog samples;
- the regular runtime log stopped at the same heartbeat while the watchdog
  continued.

Thus this run is not a replacement HOME PID or overwritten executable hook.
The hooked HOME execution path becomes dormant and the normal runtime thread
also blocks during the transition.

Read-only disassembly of the installed libctru `hid.o` identified the exact
blocking risk and correct alternative. `hidScanInput()` begins with a call to
`irrstScanInput()` before reading `hidSharedMem`. The button ring itself uses
the shared-memory index at `+0x10`, entries at `+0x28` with 16-byte stride, and
held buttons at entry `+4`.

V166 retains `hidInit()` so Rosalina receives libctru's official HID shared
memory mapping, but does not call `hidScanInput()` or the IR service. It reads
held buttons directly from that local official mapping using the disassembled
ring layout. This is materially different from the failed V163 experiment,
which copied HOME-published input through a cross-process channel with cache
coherency problems. The independent watchdog remains enabled.

V166 identifiers and artifacts:

- visible/runtime `V166` / `1.6.6`;
- marker `0x43545336`;
- framework diagnostic `0.7.6-direct-hid`;
- logs `hook-lifecycle-v166.txt`, `framework-live-v166.txt`, and
  `watchdog-v166.txt`;
- linked frame-hook offset remains `0x208`, target `0x0030537C`;
- build succeeded, size 338944 bytes, SHA-256
  `BD2DCCDE2A0B9EB1C6D5E98BE2933F5CEF0A1AFEE83D983E1E6E63C01B355AB5`.

Next test: confirm initial L+Y activation, then repeat Notifications and retry
L+Y. If post-Notifications input works but no panel appears, the next problem
is the dormant HOME display hook; the runtime and watchdog logs will now keep
advancing and distinguish that case.

### 2026-08-09 — V166 direct HID failed; V167 multi-HOME diagnostic built

The user reported neither initial nor post-Notifications L+Y worked on V166.
Logs showed a healthy initial hook but held input remained zero and render count
remained zero. The direct libctru mapping approach is therefore invalid in the
Rosalina sysmodule context despite matching `hid.o`'s ring layout; do not reuse
it. V167 restores the exact V165 `hidScanInput()` / `hidKeysHeld()` behavior,
which the user confirmed works before Notifications.

The V165 watchdog only followed the first process returned with the HOME title
ID. V167 additionally enumerates every running process whose title ID is
`0004003000008F02` once per watchdog sample. It records `home_count` and up to
four candidate PIDs, markers, and heartbeats in `watchdog-v167.txt`. This tests
whether the post-Notifications UI is a second HOME instance while the original
PID 15 remains dormant and continues to be selected by `openHome()`.

V167 identifiers and artifacts:

- visible/runtime `V167` / `1.6.7`;
- marker `0x43545337`;
- framework diagnostic `0.7.7-multi-home`;
- logs `hook-lifecycle-v167.txt`, `framework-live-v167.txt`, and
  `watchdog-v167.txt`;
- linked frame-hook offset remains `0x208`, target `0x0030537C`;
- build succeeded, size 339456 bytes, SHA-256
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`.

Next test: verify initial V167 L+Y, then repeat Notifications and retry L+Y.
The expected functional result is the known V165 behavior; the purpose of this
iteration is the post-transition multi-HOME evidence.

### 2026-08-09 — V167 proved dual HOME processes; V168 hook-local recovery built

The user confirmed V167 initial L+Y worked and post-Notifications L+Y did not.
The multi-HOME watchdog conclusively found two simultaneously listed HOME
processes:

- dormant original PID 15: marker `43545337`, heartbeat frozen at 314;
- active replacement PID 40: marker `43545337`, heartbeat advancing at 452.

Both instances were patched by loader. The failure was `openHome()` selecting
the first/oldest match and the runtime remaining attached to it. This validates
the loader's per-process static patch behavior and disproves the same-process
thread-lifecycle hypothesis.

An initial V168 implementation attempted to terminate and recreate the stale
Rosalina worker. Compilation correctly rejected it because this environment has
no `svcTerminateThread`; that approach was discarded and never deployed.

The safe V168 implementation makes two changes:

- `openHome()` now retains the highest/newest matching HOME PID rather than the
  first match, so any future worker attachment selects the active replacement;
- every patched HOME frame hook independently edge-detects raw `L+Y` (`0xA00`)
  from HOME's own HID ring and toggles its local overlay flag. Prior held state
  is stored at channel `+0xF8`. This lets a replacement HOME instance show/hide
  its panel even when the Rosalina worker remains blocked on the old instance.

This iteration deliberately does not claim full dynamic menu control after the
transition. The replacement process's entry hook already allocated and
initialized its panel, and its display hook can render that static panel. V168
tests hook-local activation first; full worker migration can be designed once
that active-process path is confirmed.

V168 identifiers and artifacts:

- visible/runtime `V168` / `1.6.8`;
- marker `0x43545338`;
- framework diagnostic `0.7.8-worker-handoff` (name predates the safe fallback);
- V168 logs use the corresponding versioned filenames;
- frame-hook offset remains `0x208`, target `0x0030537C`; stub end grew from
  loader address `0x140075F4` to `0x14007620`, still within the cave limit;
- build succeeded, size 339456 bytes, SHA-256
  `5B6A9657B216D2B13FC9DD1E97311633047B966A55A543D7AFB33129E3255E43`.

Next test: verify normal initial L+Y, enter Notifications, return, and try L+Y.
After the transition, success means at least the replacement HOME's initialized
static panel appears and closes reliably; dynamic controls are not yet expected
to be worker-driven on PID 40.

### 2026-08-09 — V168 failed due dual activation ownership; V169 built

The user reported V168 did not open after the Notifications cycle and suggested
finding a resume event. V168 logs refined the lifecycle model:

- watchdog selected PID 40 and saw it dormant at heartbeat 324;
- original PID 15 had resumed and advanced to heartbeat 2698;
- the runtime worker was still attached to PID 15 and its regular log resumed;
- both HOME processes remained patched.

Therefore Notifications temporarily starts/activates a second HOME instance,
then control returns to the original HOME PID. This is a handoff/resume cycle,
not a permanent replacement. Luma's plugin loader contains a kernel HOME event
path (`PLG_CFG_HOME_EVENT`) that emits plugin `PLG_HOME_ENTER` and
`PLG_HOME_EXIT`; this is a promising explicit lifecycle source for the eventual
plugin framework and should be investigated before relying only on polling.

The immediate V168 functional failure had a simpler cause: activation had two
owners. The resumed Rosalina worker and the HOME frame hook both detected the
same L+Y edge and each toggled channel `+0x20`. Two toggles cancel, matching the
observed final `overlay=0`. V169 removes activation toggling from Rosalina. The
HOME frame hook is now the sole L+Y edge owner on every HOME instance. Rosalina
still handles menu navigation, rendering, and sort actions once the overlay is
open.

Changed subsystem: activation ownership only. Known V167 HID input, sorting,
watchdog, multi-HOME enumeration, display hooks, and persistence code are
otherwise preserved. Expected behavior: initial L+Y should work through the
hook; after Notifications and resume, L+Y should toggle once rather than twice.
If the panel opens but controls do not respond, activation is fixed and worker
lifecycle remains. If it does not open and `overlay` remains zero, HOME-side raw
edge capture is wrong. If `overlay=1` but no panel renders, the display path is
the remaining failure.

V169 identifiers and artifacts:

- visible/runtime `V169` / `1.6.9`;
- marker `0x43545339`;
- framework diagnostic `0.7.9-hook-activation`;
- logs `hook-lifecycle-v169.txt`, `framework-live-v169.txt`, and
  `watchdog-v169.txt`;
- frame-hook offset `0x208`, target `0x0030537C`, stub end `0x14007620`;
- build succeeded, size 339456 bytes, SHA-256
  `4C6BFC54A606C13FD3594D2386B95C4FFD01F53A9F74842CA4A497AE5228FBB8`;
- last known-good initial-overlay rollback is V167, SHA-256
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`.

Test: try initial L+Y, close, open Notifications, return, wait about two seconds,
then try L+Y. Also test navigation if the panel appears and power off. Logs will
separate activation (`overlay`), render count, active PID, and heartbeat.

### 2026-08-09 — V169 regressed initial activation; V170 additive fallback built

The user reported V169 initial L+Y did not work and explicitly required keeping
the initial hook that worked while adding resume handling separately. V169 is
therefore rejected. The last confirmed initial activation remains V167's
Rosalina `hidScanInput()` edge handler.

Luma lifecycle research found that K11 watches `APT:ReceiveParameter` command
traffic and can signal `PLG_CFG_HOME_EVENT`, which Rosalina converts to
`PLG_HOME_ENTER` / `PLG_HOME_EXIT`. That mechanism is gated by current plugin
loader state, so it cannot yet be assumed to fire reliably for this static HOME
patch. It is documented as a future explicit resume integration point, not used
as the sole V170 trigger.

V170 restores the confirmed V167 initial activation exactly and makes fallback
activation additive and mutually exclusive:

- Rosalina again owns normal L+Y edge detection and toggling;
- when Rosalina maps a HOME process, it publishes worker marker `0x4354574B`
  (`CTWK`) at channel `+0xFC`;
- each HOME frame hook still has independent L+Y edge detection, but it first
  checks `+0xFC` and does nothing when a Rosalina worker is attached;
- a temporary/replacement HOME process without `CTWK` may toggle its own static
  panel, while the original/resumed worker-controlled HOME retains the proven
  path;
- channel `+0xF8` remains the hook-local previous-held latch.

This prevents V168's double toggle and V169's replacement of the working
initial path. Sorting, navigation, persistence, display hooks, watchdog, and
multi-HOME enumeration are unchanged. A necessary assembly correction reloads
the HID ring pointer after checking `CTWK`; otherwise the later down-key read
would have used the marker address as a ring pointer.

Expected behavior: initial L+Y must behave like V167. During a temporary HOME
instance with no worker, L+Y may display its initialized static panel. When the
original HOME resumes, its attached Rosalina worker handles the full menu. A
static-only panel during the transition is a partial diagnostic success, not a
complete lifecycle fix.

V170 identifiers and artifacts:

- visible/runtime `V170` / `1.7.0`;
- marker `0x43545340`;
- worker marker `0x4354574B` at channel `+0xFC`;
- framework diagnostic `0.8.0-resume-fallback`;
- logs `hook-lifecycle-v170.txt`, `framework-live-v170.txt`, and
  `watchdog-v170.txt`;
- frame-hook offset `0x208`, target `0x0030537C`, stub end `0x14007644`;
- build succeeded, size 339456 bytes, SHA-256
  `C500715CCDFB6108DA4892020733EAA9D1003F5A05DE358087C00FC4AE1FBE40`;
- last known-good initial-overlay rollback remains V167, SHA-256
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`.

Test initial L+Y first. If it fails, stop: worker marker publication is
incorrectly suppressing the restored path. If it works, close, open
Notifications, return, wait two seconds, retry L+Y, test navigation, power off,
and inspect the three logs.

### 2026-08-09 — V170 did not boot; immediate rollback to V167

V170 failed the boot safety gate. No new ARM11 dump was written. The SD was
mounted immediately, V170 was archived, and the exact previously confirmed V167
payload was restored as the sole active payload. Active rollback artifact:

- `/luma/payloads/CthulhuHomeOSD167.firm`;
- size 339456 bytes;
- SHA-256
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`.

The protected root `boot.firm` remained unchanged at SHA-256
`10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Likely source-level boot fault: V170's new assembly fallback loaded the worker
magic into `r0` inside `cthulhuFrameHook` without preserving the native frame
function's return value. Existing code deliberately used only saved scratch
registers after the native call. Clobbering `r0` can corrupt HOME startup. The
fallback also temporarily reused `r12` and required a ring-pointer reload,
increasing risk in a proven hook. Regardless of exact fault, V170 is rejected.

Repository source was restored to the V167 baseline as well as the SD payload:
no HOME-side activation fallback, no worker marker, first-match `openHome`, and
the confirmed Rosalina L+Y activation path. Do not inherit V168–V170 frame-hook
activation code in the next iteration.

Next resume work must be separate from `cthulhuFrameHook`. Preferred direction:
instrument or subscribe to the existing Luma/K11 APT HOME lifecycle signal
(`PLG_CFG_HOME_EVENT` derived from `APT:ReceiveParameter` wakeup/response
commands), preserving V167 code paths byte-for-byte. First deploy should be
logging-only and prove which enter/exit/resume events fire for Notifications;
only then should the event trigger worker migration or reattachment.

V167 source restoration was verified reproducibly. The first verification build
did not match because V168-only `runtimeAttachedPid` bookkeeping remained in the
logger source. That global, watchdog field, assignment, and forward declaration
were removed. A second clean incremental rebuild produced SHA-256
`7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`,
exactly matching the archived and active V167 payload. The repository build
mirror and canonical runtime source are now byte-for-byte reproducible at the
known-good rollback point.

### 2026-08-09 — APT lifecycle diagnostic V171 built on exact V167 hook

V171 is a logging-only Rosalina diagnostic built after reproducing the exact
V167 payload hash. The HOME loader/static stub, marker `0x43545337`, frame hook,
overlay input, rendering, sorting, persistence, and watchdog behavior remain
V167. No HOME assembly or activation code changed.

The only new behavior is in Rosalina's existing
`PluginLoader__HandleKernelEvent`. Immediately after reading Luma's plugin
configuration event, it calls
`CthulhuRuntimeLogger_RecordAptHomeEvent(rawEvent, status, pluginIsHome,
target)`. That function writes `/3ds/Cthulhu/apt-home-event-v171.txt` with:

- event sequence count and raw event value;
- whether the value equals `PLG_CFG_HOME_EVENT` (`0x00010000`);
- current plugin status and pre-handler `pluginIsHome` state;
- plugin target handle, PID lookup result, and target PID.

The purpose is to prove whether Notifications generates Luma's existing APT
HOME lifecycle event before using it for resume recovery. Absence of the file
after the test means this plugin-scoped signal is not delivered in the static
HOME configuration; that is also a useful result. This diagnostic does not try
to fix post-Notifications activation.

Version visibility is deliberately explicit about the unchanged hook: the HOME
overlay continues to show `V167`, while Rosalina's diagnostics menu label is
`Check HOME OSD V167 / APT V171`. The chainload filename is V171.

Artifacts:

- build succeeded;
- HOME stub start/frame/end remain `0x14007024`, `0x1400722C`, and
  `0x140075F4`, exactly the V167 layout;
- payload size 339968 bytes;
- SHA-256
  `392FFC890E93AECB381897BDA96333AB395FB00E570436440421395DFFA58721`;
- rollback payload remains exact V167 SHA-256
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`.

Test: confirm the overlay visibly remains V167 and initial L+Y still works;
close it, open Notifications, return HOME, try L+Y once, power off, and inspect
`apt-home-event-v171.txt` plus the unchanged V167 watchdog/runtime logs. Initial
activation failure or boot failure is an unexpected Rosalina-only regression
and requires immediate rollback without modifying HOME hooks.

### 2026-08-09 — V171 signal absent; researched hooks and built V172 kernel wakeup log

Clarification from the user: Notifications opened and returned normally, but
the shortcut did not work after returning. V171's initial V167 shortcut worked.
No `apt-home-event-v171.txt` was created. The watchdog still saw two patched
HOME processes, proving that Luma's plugin-scoped `PLG_CFG_HOME_EVENT` was not
forwarded in this static setup.

Internet/source research findings:

- 3dbrew documents `APT:ReceiveParameter` response word 3 as the command/signal
  describing why the applet woke;
- command 10 is `COMMAND_WAKEUP_BY_EXIT`, 11 is wakeup-by-pause, 12 is
  wakeup-by-cancel, with further wakeup commands through 17;
- 3dbrew notes New3DS system-applet behavior can leave HOME resident or
  terminate/relaunch it depending on applet exit behavior, matching the observed
  PID 15/PID 40 transitions;
- Luma K11 already intercepts successful `APT:ReceiveParameter` responses but
  gates `PLG_CFG_HOME_EVENT` on normal plugin state;
- targeted GBATemp searches did not reveal a public, more complete HOME Menu
  resume-hook implementation, and direct automated access to the referenced
  thread is blocked with HTTP 403.

V172 adds a Cthulhu-only logging signal at the existing K11 interception point.
For APT wakeup commands 10 through 17, K11 signals event
`0x80000000 | (command << 16)`. Rosalina recognizes the high-bit namespace,
logs the exact command, clears the custom event while preserving the low plugin
status, publishes the required `0x1002` acknowledgement, and returns before all
normal plugin HOME handling. It therefore does not toggle `pluginIsHome`, swap
memory, notify a plugin, or change plugin-loader lifecycle state.

Changed subsystems: K11 APT response observation and Rosalina custom-event log
only. HOME loader/stub, marker, initial shortcut, renderer, sorter, persistence,
and watchdog remain the verified V167 versions. This is diagnostic, not a
resume fix. The custom event uses Luma's existing synchronous event/ack path,
which is a kernel-touching risk; immediate rollback is required for any boot,
applet-launch, return, shutdown, or power-off regression.

Version/artifacts:

- chainload/diagnostic version V172; overlay remains visibly V167;
- Rosalina menu label `Check HOME OSD V167 / APT V172`;
- log `/3ds/Cthulhu/apt-home-event-v172.txt` now includes custom-event flag and
  decoded `apt_command`;
- HOME stub addresses remain exact V167 values `0x14007024`, `0x1400722C`, and
  `0x140075F4`;
- build succeeded, size 339968 bytes, SHA-256
  `71C13FB6A06AC63B5B0D371CF31C662916282D79FF05B9F2131A1074981DCA36`;
- rollback remains exact V167 SHA-256
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`.

Test: boot, confirm initial V167 L+Y, close, open Notifications, return, try L+Y,
test power off, and reinsert the card. The key result is
`apt-home-event-v172.txt`; its `apt_command` identifies the separate resume
trigger. If absent, the relevant transition does not pass through the existing
K11 ReceiveParameter interception and the next target is NS process launch/
termination observation rather than further HOME-hook changes.

### 2026-08-09 — V172 custom APT signal absent; V173 read-only probe built

The user confirmed the shortcut still failed only after returning from
Notifications. V172 produced no `apt-home-event-v172.txt`, although initial
V167 activation worked and two patched HOME processes were again observed.
Therefore this Notifications path does not cross Luma's existing successful
`APT:ReceiveParameter` interception for commands 10–17. The V172 K11 custom
event and Rosalina handler were removed from the build; do not build recovery
on that path.

Further disassembly review corrected an important V166 mistake. libctru's
button-held value is loaded from the current HID ring entry at entry offset
`+0`, while V166 sampled entry `+4`. Thus V166's zero input did not disprove
direct official HID-memory sampling; it tested the wrong field. Per the user's
instruction, the working V167 shortcut is still not replaced.

V173 adds a separate read-only Rosalina input probe thread. It waits for the
existing runtime to initialize HID, then samples libctru's official local
`hidSharedMem` every 16 ms using index `+0x10`, table `+0x28`, stride `0x10`,
held field `+0`. It never calls HID/IR services and never writes HOME or overlay
state. It latches current held, last nonzero held, transition count, and L+Y
edge count.

The independent watchdog also now compares per-PID heartbeat deltas and latches
the most recently advancing HOME PID as `active_home_pid`. It writes the input
probe fields and active PID to `/3ds/Cthulhu/watchdog-v173.txt`. Together these
values show whether L+Y is visible outside the blocked worker and which HOME
process owns the active frame loop at that moment.

Changed subsystem: additive read-only diagnostics in Rosalina only. V172 K11
changes were reverted. HOME loader/stub, marker, initial activation, rendering,
sorting, persistence, and HOME addresses remain exact V167. Expected behavior
is unchanged: initial shortcut works; post-Notifications shortcut likely still
fails because V173 only records evidence.

Version/artifacts:

- chainload/probe V173; overlay remains visibly V167;
- Rosalina menu label `Check HOME OSD V167 / Probe V173`;
- HOME stub addresses remain `0x14007024`, `0x1400722C`, `0x140075F4`;
- build succeeded, size 339968 bytes, SHA-256
  `AA921676E6D230EA8B4303F81AA5C2B8B2471CF25EDA29705AEE451AD96268AE`;
- exact V167 rollback remains
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`.

Test: initial L+Y, close, Notifications and return, press L+Y deliberately for
about one second, power off, reinsert. Interpret `probe_chord_edges > 0` as
proof that nonblocking Rosalina input survives the transition. `active_home_pid`
identifies the process a future separate controller must attach to. A zero edge
count means the official HID mapping itself becomes stale and requires a new
HID initialization/lifecycle strategy.

### 2026-08-09 — V173 proved input survives on active PID 40; V174 recovery built

The user reported the functional shortcut still did not work after
Notifications, as expected for the logging-only V173 build. The probe supplied
the decisive evidence:

- `probe_chord_edges=4`: the separate nonblocking probe saw L+Y reliably;
- `probe_transitions=18`: official HID shared memory remained live;
- `active_home_pid=40`: PID 40 owned the advancing frame loop when the chord was
  sampled;
- the original V167 runtime remained attached to PID 15;
- both HOME processes retained valid V167 markers and hooks.

Therefore neither the physical input nor the static hook is lost. The controller
is attached to the wrong HOME instance during the system-applet transition.

V174 adds a separate recovery controller inside the read-only probe thread. The
primary V167 worker records its initial PID (normally 15) but its activation code
is unchanged. On an L+Y edge, the probe does nothing when the active heartbeat
PID equals the primary PID, preserving the exact working initial path. Only when
the active PID is different does it:

1. open that exact PID;
2. map only the V167 channel page at a dedicated local address;
3. validate marker `0x43545337` and a nonzero initialized panel;
4. toggle only channel overlay word `+0x20`;
5. flush both mapped caches and unmap/close immediately.

This does not edit HOME executable code, change the static hook, move layout
data, or call sorting/persistence code. The expected post-Notifications result
is the active PID's statically initialized panel; dynamic navigation is not yet
migrated and is explicitly not claimed. Initial PID 15 behavior must remain
identical to V167.

The V174 watchdog log records primary PID, recovery attempt/success counts,
result, target PID, and overlay before/after values. Any mapping validation
failure leaves the active process unchanged. Power-off behavior remains a
required safety test.

Version/artifacts:

- chainload/recovery V174; HOME overlay itself remains V167;
- Rosalina label `Check HOME OSD V167 / Recovery V174`;
- log `/3ds/Cthulhu/watchdog-v174.txt`;
- HOME stub addresses remain exact V167 values `0x14007024`, `0x1400722C`, and
  `0x140075F4`;
- build succeeded, size 340480 bytes, SHA-256
  `AD64DE83DBB6FF778E7DE72AE8EAEF9E65034A259814AEB5B9F81019DD8CF4FC`;
- rollback remains exact V167
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`.

Test: confirm initial L+Y opens the normal dynamic menu; close it; open
Notifications and return; wait two seconds; press L+Y once. A static V167 panel
is the expected partial recovery. Press L+Y again to confirm it closes. Dynamic
navigation after transition is not expected yet. Test power off and reinsert.

### Iteration documentation requirement (explicit user request)

After every future iteration, append—not rewrite—a handoff entry that states:

1. the exact subsystem and source behavior changed;
2. what previously confirmed behavior was preserved, replaced, or regressed;
3. the evidence and reasoning that justified the change;
4. the expected user-visible behavior, including deliberately incomplete parts;
5. safety risks and mitigations, especially power-off and Launcher/SaveData risk;
6. build version, marker, filenames, size, hash, and deployment state;
7. the last known-good rollback payload;
8. the exact test sequence and what each possible result would mean.

Do not describe a diagnostic as a completed functional fix. If an implementation
changes a previously working subsystem, call that out before deployment and
state why the change is necessary. Prefer additive diagnostics around proven
behavior; only replace proven behavior after direct evidence identifies it as
the cause.

### 2026-08-09 — V175 normalizes the recovered overlay's text size

The user confirmed that V174 successfully opened the overlay after returning
from Notifications, but reported that its text was much larger than the normal
menu. The V174 watchdog log proved recovery targeted active PID 40 and toggled
its overlay successfully. The large text was the expected boot-time diagnostic
panel produced by the unchanged static V167 stub; the compact menu had only
been rendered into PID 15's panel by the original Rosalina runtime worker.

V175 changes only the separate post-resume recovery controller in
`runtime/luma/source/cthulhu_runtime_logger.c`. When L+Y targets a non-primary
active HOME PID and that PID's overlay is currently closed, the controller now
maps its already validated panel at dedicated local address `0x00800000`, calls
the existing compact V167 `renderMenu` with safe default state (A-Z and folders
before titles), flushes both caches, unmaps the panel, and only then toggles the
overlay. Closing an already-open recovered panel does not redraw it. If panel
mapping fails, V175 records the result and refuses to expose stale fallback
graphics.

The HOME loader, static stub, frame/input hooks, marker `0x43545337`, initial
PID behavior, input suppression, sorting implementation, persistence, and HOME
save data are unchanged from the proven V167 base. The V174 active-PID recovery
logic is preserved. This iteration deliberately does not migrate navigation or
sorting control to the replacement PID: after Notifications the compact panel
should open and close, but its fields are not expected to respond yet. Initial
HOME behavior should remain fully interactive exactly as before.

Diagnostics were bumped to `/3ds/Cthulhu/watchdog-v175.txt`. New fields
`recovery_render_result` and `recovery_render_count` distinguish a render/map
failure from a visibility-toggle failure. A zero render result and increasing
count mean the compact panel was written. This remains an additive Rosalina
mapping operation; it does not write Launcher/SaveData. The mapping is bounded
to the stub-published 128 KiB allocation and is always unmapped before the
process handle closes. Power-off remains a required safety check.

Version/artifacts and deployment:

- chainload/recovery V175; compact panel title intentionally remains
  `CTHULHU SORT V167` because the HOME stub/base renderer is still V167;
- Rosalina label `Check HOME OSD V167 / Recovery V175`;
- active payload `I:\luma\payloads\CthulhuHomeOSD175.firm`;
- size 340480 bytes, SHA-256
  `2E5D84695831C8F389310E16BFC3AF231A303E88CFF5C29DD2AF0BB85C6D071E`;
- V174 was archived to
  `I:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD174.firm`;
- exact V167 rollback remains SHA-256
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`;
- protected root `I:\boot.firm` was verified unchanged before and after deploy
  at SHA-256
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`;
- only V175 is active in `I:\luma\payloads`.

Test sequence: chainload V175 and verify the Rosalina V175 label; confirm initial
L+Y still opens the normal compact interactive menu and closes it; open
Notifications and return; wait two seconds; press L+Y. The recovered panel
should now use the same small compact text and should close on a second L+Y.
Navigation after Notifications is not yet expected. Finally verify power off.
If the large diagnostic text remains, inspect `watchdog-v175.txt`: a failed
`recovery_render_result` identifies mapping failure, while zero with a positive
`recovery_render_count` means the render succeeded and the active-panel identity
needs re-evaluation. Any initial-overlay or power-off regression means roll back
immediately to exact V167.

### 2026-08-09 — Live-sort work resumed; process-reload test selected

The user confirmed V175's recovered panel now has the correct compact text and
asked to resume sorting without reboot. No payload or source behavior was
changed in this planning/diagnostic iteration; V175 remains the sole active
payload and all V167/V175 confirmed behavior is preserved.

Review of the existing live-sort path confirmed that a successful overlay sort
already commits and verifies extdata, copies the planned raw and processed grid
into HOME, and asks the V167 frame hook to call native rebuild `0x0013C680` and
publish `0x00146D10`. Those calls previously returned and acknowledged but did
not move visible icons, proving the missing boundary is the already-instantiated
visible icon-object model rather than persistence or raw/grid generation.
Existing attempted controller capture/replay through `0x001BA594` did not
naturally execute and must not be presented as a working refresh.

V175's lifecycle evidence provides a new low-risk discriminator: Notifications
creates/activates another fully patched HOME process. The next test is to apply
a visibly opposite sort, remain in the same boot, close the overlay, enter
Notifications, and return. If the icons appear in the new order, HOME process
activation/reload is a proven no-hardware-reboot refresh boundary and can be
instrumented/automated next. If they remain unchanged, the replacement HOME
inherits or retains the old icon-object model, and work must return to tracing
the higher-level controller path (callers `0x001B9570`, `0x001B9F54`, and
`0x001B9FB8`) rather than retrying raw rebuild/publish.

This test does write the same verified sort transaction as the already proven
sorter; backups and committed read-back remain in effect. It does not add new
Launcher/SaveData writes or hooks. Version/artifacts remain V175, 340480 bytes,
SHA-256
`2E5D84695831C8F389310E16BFC3AF231A303E88CFF5C29DD2AF0BB85C6D071E`.
The exact V167 rollback remains
`7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`.

### 2026-08-09 — Notifications did not refresh icons; V176 controller scan deployed

The user applied a sort successfully, entered Notifications, and returned. The
icons did not change, but the overlay worked before and after Notifications.
This disproves process/applet transition as a sufficient icon-layout reload and
confirms V175 lifecycle recovery remains stable.

The SD logs showed a clean persistent transaction: 173 mutations, result zero,
native rebuild request/ack `1`, native result zero, but `layout_object=0` and
`layout_refresh_calls=0`. Therefore raw layout generation, extdata commit,
native raw-to-grid rebuild, and publish all completed; the missing operation is
still reconstruction of HOME's already-instantiated visible icon objects.

V176 adds `ScanVisibleControllerCandidatesV176` to
`runtime/luma/source/home_menu_diagnostics.c`. Immediately after a successful
background sort, while the outer transaction already has HOME threads paused,
it performs a read-only scan of HOME's readable/writable regions. Candidate
objects must match fields used by the native high-level layout routine around
`0x001B9534`: active page at `+0x81C` in range 0..7, plausible icon model at
`+0x7B0`, plausible data/UI pointers at `+0x08/+0x0C`, and at least one
plausible controller buffer at `+0x50/+0x54`. The scan records at most 64
candidates in `/3ds/Cthulhu/live-controller-v176.txt`, unmaps every region, and
does not write HOME memory or call any candidate function.

This is deliberately a diagnostic, not a completed live-sort fix. V175's
post-Notifications recovery, exact V167 HOME stub and initial menu, input
suppression, rendering, persistent sorter, folder behavior, and shutdown safety
are preserved. Expected visible behavior is unchanged: sorting still commits
but icons need not move. The new file should provide an exact controller object
for a later guarded frame-bound invocation. Zero candidates means the structural
predicate is too strict or the object lives in a differently permissioned
region; multiple candidates will be narrowed using pointer ownership and active
page evidence before any native call.

Safety: V176's new operation is bounded and read-only, runs only after an
already-successful transaction, and caps both mapped region size (64 MiB) and
logged candidates. Existing SaveData/Launcher backup and commit behavior is
unchanged. The user must still verify normal power off after the test.

Version/artifacts and deployment:

- Rosalina label `Check HOME OSD V167 / Live Scan V176`;
- active payload `I:\luma\payloads\CthulhuHomeOSD176.firm`;
- size 341504 bytes, SHA-256
  `4957074076ED8E0DCB147E4E2869E3115595DFEF48D2C321738E2B1F10EE994C`;
- V175 archived to
  `I:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD175.firm`;
- exact V167 rollback SHA-256 remains
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`;
- protected root `I:\boot.firm` remained unchanged before/after deployment at
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`;
- only V176 is active in the payload directory.

Test: chainload V176 and verify its Rosalina label; open L+Y, choose the opposite
direction, and apply once. The progress display may remain on the controller
scan for several seconds. A normal success screen is expected, but visible icon
movement is not. Verify the overlay still closes and power off works, then
reinsert the SD. Inspect `live-controller-v176.txt`: one strong candidate gives
the next guarded refresh target; zero requires predicate refinement; many
requires additional read-only discrimination. Any hang, crash, or power-off
regression requires immediate rollback to V175 or exact V167.

### 2026-08-09 — V176 scan too broad; V177 exact class scan deployed

V176 completed safely and scanned 20,484,096 bytes across 11 HOME regions, but
returned the 64-candidate cap. Inspection showed obvious false positives such
as `0x3F800000` float tables and the compact panel's `0x20202020` fill. The
transaction itself again succeeded with 173 mutations and native rebuild ack,
while `layout_object` and `layout_refresh_calls` remained zero. No new crash
dump appeared.

Offline analysis of `HomeMenu-USA-code.bin` found the address of high-level
routine `0x001B9534` stored at `0x00308D90`, inside the `lau_title_menu` class
table beginning at `0x00308D80`. The adjacent static string is
`lau_title_menu`. This supplies a direct runtime type discriminator: the real
object using that virtual method should carry vtable pointer `0x00308D80` in
its first word.

V177 replaces only V176's loose candidate predicate. It now scans for exact
first word `0x00308D80` and logs the matching object's address plus fields at
`+0x08`, `+0x0C`, `+0x50`, `+0x54`, `+0x7B0`, and `+0x81C` to
`/3ds/Cthulhu/live-controller-v177.txt`. It remains read-only and makes no
native controller call. All V175 lifecycle behavior, V167 HOME hooks, overlay,
input suppression, sorting, persistence, folders, and power-off behavior are
preserved. Sorting is still not expected to move live icons in this diagnostic
iteration.

Safety and interpretation: an exact single match provides a strongly typed
object for the next guarded frame-bound test. Zero means the object uses an
adjusted/subobject vtable or is not resident in scanned writable memory; more
than one means ownership/active-page validation is still required. No result
authorizes a blind call. The scan retains V176's region/candidate bounds and
unmaps every region.

Version/artifacts and deployment:

- Rosalina label `Check HOME OSD V167 / Live Scan V177`;
- active payload `I:\luma\payloads\CthulhuHomeOSD177.firm`;
- size 341504 bytes, SHA-256
  `919738FFA0A29556A939139DB83760E9A906F55D3919CF6486B246A9F4480065`;
- V176 archived to
  `I:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD176.firm`;
- exact V167 rollback remains
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`;
- protected root `I:\boot.firm` stayed unchanged before/after deployment at
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`;
- only V177 is active.

Test one opposite-direction sort, confirm a normal success result and power off,
then reinsert the SD. Read `live-controller-v177.txt` automatically. Visible
icon movement is not expected yet. Any hang/crash/power-off regression requires
rollback to V175 or exact V167.

### 2026-08-09 — V177 found no vtable object; V178 direct observation hook deployed

V177 completed safely but found zero objects whose first word was `0x00308D80`.
The sort again completed normally with 173 mutations; `layout_object=0` and
`layout_refresh_calls=0`; no new crash appeared. This shows `0x00308D80` is a
static callback/resource descriptor associated with `lau_title_menu`, not a
conventional vtable stored directly in the runtime object. Do not use the V177
predicate or call an inferred candidate.

V178 replaces inference with direct observation. Loader patching now verifies
the original instruction `0xE590181C` at high-level HOME routine entry
`0x001B9534`, then branches to new `cthulhuVisibleControllerHook`. The hook:

1. preserves incoming `r2` and `r12`;
2. records the native incoming `r0` object at channel `+0xF8`;
3. increments a call count at `+0xFC`;
4. restores registers;
5. replays the displaced `ldr r1,[r0,#0x81C]`;
6. resumes at `0x001B9538`.

It never invokes the routine, changes its arguments, writes its object, or
requests a refresh. `framework-live-v167.txt` now includes
`visible_controller` and `visible_controller_calls`. The exact V167 initial
hook behavior, V175 recovery behavior, overlay, rendering, input suppression,
persistent sorting, folders, and power-off flow are otherwise preserved. V178
is still diagnostic; live icon movement is not expected.

The relocated stub now spans loader symbols `0x14007024..0x1400761C`, mapping to
remote `0x00305174..0x0030576C`, safely below cave limit `0x00306000`. The new
entry patch is signature-gated, so an unsupported HOME image fails closed rather
than writing an unknown instruction. Direct hook risk is higher than V177's
Rosalina-only scan because a native HOME entry point is changed; mitigation is
the minimal replay/continue trampoline and mandatory boot, overlay, and
power-off tests before any function call is added.

Version/artifacts and deployment:

- Rosalina label `Check HOME OSD V167 / Direct Hook V178`;
- active payload `I:\luma\payloads\CthulhuHomeOSD178.firm`;
- size 341504 bytes, SHA-256
  `5099669B8A32591C8A044CD5C12A5ED50C3C4198E0C243F860479FBF4CEECF5A`;
- V177 archived in `I:\luma\disabled\CthulhuFrameworkHistory`;
- exact V167 rollback remains
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`;
- protected root `I:\boot.firm` remained unchanged before/after deployment at
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`;
- only V178 is active.

Test: chainload V178. First confirm HOME boots, the overlay opens/closes, and
power off works. To encourage HOME's own routine without sorting, move between
HOME pages and open/close a folder once. Then perform one opposite-direction
sort, close the overlay, power off, and reinsert. Read
`framework-live-v167.txt`. A nonzero controller with positive calls is direct
evidence for the next guarded frame-bound call. Zero means this native routine
is not exercised by those UI paths and requires hooking its dispatch/owner
construction instead. Any boot/crash/power-off regression requires immediate
rollback to V175 or exact V167.

### 2026-08-09 — V178 routine dormant; V179 hooks active root constructor

V178 booted, sorted, and shut down without a new crash, but
`visible_controller=0` and `visible_controller_calls=0` even after page movement
and folder activity. The high-level `0x001B9534` callback is not naturally
dispatched on those paths, so its entry hook cannot discover the owner.

Further disassembly identified the actual large root-object constructor at
`0x002D1144`. It calls a base constructor at `0x001EEC00`, then at
`0x002D1150` loads primary type pointer `0x0032258C` and stores it to the new
object. The same object later initializes fields `+0x7B0` and `+0x81C`. The
previous `0x00308D88` descriptor is used only for embedded callback subobjects
near `+0x7DC`, explaining V177's failed top-level-vtable assumption.

V179 adds `cthulhuRootControllerCtorHook` at `0x002D1150`, signature-gated on
original instruction `0xE59F15A0`. It records constructor return `r0` and a call
count in the existing channel `+0xF8/+0xFC`, restores registers, replays the
displaced load as `ldr r1,=0x0032258C`, and resumes at `0x002D1154`. It does not
write the object, invoke refresh, or change constructor arguments. V178's
dormant routine-entry observer remains installed but is behavior-neutral.

Expected behavior: HOME should boot normally and the framework log should show
a nonzero visible controller with at least one call without needing a sort.
Live icons are still not expected to move. All V167/V175 overlay, recovery,
input suppression, persistent sort, folder, and power-off behavior is preserved.
The relocated stub ends at loader symbol `0x14007648` (remote `0x00305798`),
below the `0x00306000` cave limit. The constructor hook is a new native entry
patch and must be rolled back on any boot/power regression.

Version/artifacts and deployment:

- Rosalina label `Check HOME OSD V167 / Root Hook V179`;
- active payload `I:\luma\payloads\CthulhuHomeOSD179.firm`;
- size 341504 bytes, SHA-256
  `7C2E0AB7FADF230B67F0642E6D5B0FC3E6C9C80DC3D144284140FE267284B098`;
- V178 archived in `I:\luma\disabled\CthulhuFrameworkHistory`;
- exact V167 rollback remains
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`;
- root `I:\boot.firm` remained protected and unchanged at
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`;
- only V179 is active.

Test only boot, verify the V179 label, open/close L+Y, wait five seconds, power
off, and reinsert. No sort is needed. Read `framework-live-v167.txt`. A nonzero
controller/call count enables validation of the object and then a separately
guarded frame-bound refresh experiment. Zero means the selected constructor is
not the active instance path. Any boot/crash/power-off regression requires
rollback to V175 or exact V167.

### 2026-08-09 — V179 constructor dormant; V180 observes active icon model

V179 booted and shut down safely but again logged zero controller calls. The
selected large-root constructor is a dormant variant on this HOME path. Further
constructor guessing was abandoned.

The proven central frame routine calls multiple subsystem updates every frame.
Static tracing of ordinary page/folder navigation found repeated calls to small
native query `0x001D0424`, with the live icon-model pointer passed directly in
`r0` and page/index in `r1`. Numerous active title-menu routines obtain this
same model from owner offset `+0x7B0` before calling the query. Therefore a
captured model pointer can identify its containing owner deterministically by
finding the exact pointer at `owner+0x7B0`.

V180 adds signature-gated `cthulhuIconModelQueryHook` at `0x001D0424`, verifying
original instruction `0xE3510008`. It records incoming `r0` and call count at
channel `+0x100/+0x104`, preserves `r2/r12`, replays `cmp r1,#8`, and resumes at
`0x001D0428`. The framework log adds `icon_model` and
`icon_model_query_calls`. The hook does not change the model or query result and
does not invoke a refresh. V178/V179 dormant observers remain behavior-neutral.

Expected behavior is unchanged and live icon movement is not yet claimed.
Navigate across pages and open/close a folder so the query executes. A nonzero
model and positive call count enables a read-only owner back-reference scan,
followed by a separately guarded refresh iteration. All V167/V175 overlay,
resume recovery, input suppression, persistent sorting, folder handling, and
power-off behavior remain preserved.

The relocated stub ends at loader `0x14007670` (remote `0x003057C0`), below
`0x00306000`. Safety risk is another native entry hook; mitigation is a
six-instruction observer that preserves scratch state and exactly replays the
displaced comparison. Roll back on any boot, navigation, or power regression.

Version/artifacts and deployment:

- Rosalina label `Check HOME OSD V167 / Model Hook V180`;
- active `I:\luma\payloads\CthulhuHomeOSD180.firm`;
- size 341504 bytes, SHA-256
  `6FE8E74BAC1FE803384E880DBC365E5D039AF9A73B982FDE1871B75B9CDC6AE1`;
- V179 archived in `I:\luma\disabled\CthulhuFrameworkHistory`;
- exact V167 rollback remains
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`;
- protected root `I:\boot.firm` remained unchanged at
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`;
- only V180 is active.

Test: boot V180, verify label and overlay, navigate left/right across several
pages, open and close a folder, wait two seconds, power off, and reinsert. No
sort is required. Read the new icon-model fields automatically. Zero calls
means the presumed navigation path is still inactive on this build; positive
calls provide the active model. Any crash or power-off failure requires rollback
to V175 or exact V167.

### 2026-08-09 — V180 inactive; V181 scans exact live-layout back-references

V180 booted and powered off safely, but `icon_model_query_calls=0`. Together
with V178/V179, this proves the `0x001B/0x001D/0x002D` function family selected
from static similarities is not the active title-menu implementation in the
tested HOME mode. Further hooks in that family were abandoned.

The proven central frame dispatcher `0x00102298` is active but invokes global
subsystem managers rather than passing a useful root pointer. V181 therefore
uses a deterministic data-ownership test after a successful sort: the sorter
already discovers exact live raw and processed-grid addresses. A new read-only
`ScanLiveLayoutBackrefsV181` scans writable HOME regions for words exactly equal
to `g_lastRawAddress` (`0x346CA1E0` on this console) or
`g_lastProcessedAddress` (`0x346CCF90`). For every match it records the pointer
location and four surrounding words on each side to
`/3ds/Cthulhu/layout-backrefs-v181.txt`. It caps results at 96, mapped region
size at 64 MiB, and unmaps every region.

This replaces the superseded V177 scanner in the post-sort path. It does not
write HOME memory or call any native routine. V178-V180 observers remain
installed but have shown zero calls and are behavior-neutral. Sorting still is
not expected to move icons live. A back-reference inside an active manager or
heap object supplies a concrete owner to correlate with the central frame graph;
only the known global wrapper references means the visible model does not retain
raw/grid pointers and a different boundary must be traced.

All V167/V175 overlay, recovery, input suppression, sorting, folders,
persistence, and power-off behavior is preserved. Existing backup/commit safety
is unchanged. V181 requires a sort because the exact dynamic addresses are set
during discovery. The scan is read-only but may add several seconds to the
success path.

Version/artifacts and deployment:

- Rosalina label `Check HOME OSD V167 / Backrefs V181`;
- active `I:\luma\payloads\CthulhuHomeOSD181.firm`;
- size 341504 bytes, SHA-256
  `F7E5775E3AC5477F153448BDE9BE4EC00402933935AB5D938693960460797667`;
- V180 archived in `I:\luma\disabled\CthulhuFrameworkHistory`;
- exact V167 rollback remains
  `7E02E688915891BE6954D3581026519AE6E78348D0F797F56F79C86632419BAB`;
- protected root `I:\boot.firm` stayed unchanged before/after deployment at
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`;
- only V181 is active.

Test: boot V181, verify label and overlay, apply one opposite-direction sort,
allow the read-only scan to finish, verify normal success and power off, then
reinsert. Read `layout-backrefs-v181.txt` automatically. Live icon movement is
not expected. Any crash or shutdown regression requires rollback to V175 or
exact V167.

### 2026-08-10 — V181 crashed after scan; active payload rolled back to V175

The user reported a crash during the V181 test. The SD initially remounted as a
stale inaccessible `I:` reader slot, then correctly appeared as `H:`. No writes
were attempted while the filesystem was inaccessible.

The new dump was preserved locally as
`runtime/.analysis/crash-v181-layout-backrefs.dmp`, size 1548 bytes, SHA-256
`58F3D357A89ACEAD042893C60845BA052FD3BD0217AE833D05530CDB11CFC787`.
The on-card dump is `H:\luma\dumps\arm11\crash_dump_00000001.dmp`.

`layout-backrefs-v181.txt` was fully written before the crash and reported:

- raw `0x346CA1E0`, processed `0x346CCF90`;
- 11 regions and 5,121,024 words scanned;
- zero raw pointer references and zero processed-grid pointer references.

The journal reached `ready-for-graceful-reboot` with result zero, so the
persistent transaction had committed before the crash. Because the scan
completed and the failure occurred afterward, the leading hypothesis is that
mapping remote HOME regions at their original virtual addresses collided with
or displaced Rosalina mappings and caused latent state corruption. Regardless
of precise dump decoding, V181 is unsafe and must not be redeployed. The zero
back-reference result also shows this scan did not identify a live owner.

Rollback performed:

- V181 moved out of active payloads to
  `H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD181-CRASHED.firm`;
- known-stable V175 restored as the sole active payload at
  `H:\luma\payloads\CthulhuHomeOSD175.firm`;
- active V175 size 340480 bytes, SHA-256
  `2E5D84695831C8F389310E16BFC3AF231A303E88CFF5C29DD2AF0BB85C6D071E`;
- protected root `H:\boot.firm` verified unchanged before and after rollback at
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Important source/deployment distinction: canonical and build-tree sources still
contain V181 experiments and are **not** equivalent to the active safe payload.
Do not rebuild/deploy current source without first removing the V178-V180 dormant
hooks and the V181 broad same-address scan, or restoring source to a safe V175
baseline. Active device behavior is now V175 (V167 HOME base plus V175 recovered
compact panel).

Next test is only a rollback smoke test: chainload V175, confirm initial L+Y and
post-Notifications L+Y, and verify power off. Do not run another sort until HOME
state is confirmed normal. Future live-refresh diagnostics must avoid mapping
arbitrary remote regions at identical local virtual addresses; use a reserved
window one region/page at a time or HOME-side bounded instrumentation, with
explicit collision checks and no native call until ownership is proven.

### 2026-08-10 — V181 dump fully decoded: stack overflow, not mapping collision

The initial post-crash mapping-collision explanation above was a cautious
hypothesis made before decoding the dump. Full decoding against Luma's
`ExceptionDumpHeader`, the exact V181 Rosalina ELF, and its link map identifies
a definitive different cause.

Dump facts:

- ARM11 core 0, exception type 3 (data abort);
- PC `0x14024368`, LR `0x14024354`;
- both addresses resolve inside libctru `FSFILE_Write` in the V181 Rosalina ELF;
- PC is the conditional `strne r3,[r1]` that writes the returned byte count;
- `R1=0x62653D6B`, an invalid address whose little-endian bytes are ASCII
  `k=eb`, content originating in the generated textual report;
- DFSR low status is 5 (translation fault, section), with FAR also
  `0x62653D6B`;
- current process is Rosalina (PC/link-map range), not HOME.

Root cause: `ScanLiveLayoutBackrefsV181` declared `char report[12288]` as a local
stack buffer. The Rosalina runtime thread has only `runtimeStack[0x2000]` (8192
bytes), before accounting for caller frames and other locals. The report buffer
therefore overflowed the thread stack and replaced the `IFile_Write` output
pointer/call state with report text. `FSFILE_Write` later tried to store the
byte count through corrupted `R1`, producing the data abort. The dump directly
supports stack corruption; it does **not** support the earlier mapping-collision
hypothesis.

The V181 scan result itself remains zero raw and zero grid back-references across
the scanned regions, but no safety conclusion about the same-address mapping
scheme should be drawn from this crash. If a future diagnostic reuses this scan,
the report must be static/global or heap-backed and tightly bounded; the thread
stack must never hold multi-kilobyte diagnostic arrays. Prefer a reserved local
mapping window as an independent hardening measure, but do not misattribute this
specific crash to mapping.

No new payload was deployed during this analysis. V175 remains the sole active
safe payload on `H:` with SHA-256
`2E5D84695831C8F389310E16BFC3AF231A303E88CFF5C29DD2AF0BB85C6D071E`.

### 2026-08-10 — V182 fixes V181 crash and restores proven HOME hook footprint

V182 was implemented, built, verified, and deployed as the sole active payload.
It fixes the decoded V181 stack overflow rather than merely masking the crash:

- the 12 KiB back-reference report is now file-scope static storage, not a
  local variable on Rosalina's 8 KiB runtime-thread stack;
- the scanner maps HOME memory in bounded 64 KiB chunks at the dedicated local
  window `0x00900000`, scans it, and unmaps each chunk; it no longer maps remote
  regions at their original HOME virtual addresses;
- output is now `/3ds/Cthulhu/layout-backrefs-v182.txt` and identifies itself as
  scan version 1.8.2;
- the dormant V178-V180 visible-controller, root-controller-constructor, and
  icon-model-query hooks and their logger fields were removed from both the
  canonical and build-tree sources;
- the visible Rosalina menu label is
  `Check HOME OSD V167 / Backrefs V182`.

The loader link map proves the injected HOME stub is back to the exact known-good
V167/V175 footprint: start `0x14007024`, frame hook `0x1400722C`, and end
`0x140075F4`. The build completed successfully in the existing Docker toolchain.

Deployment state:

- sole active payload:
  `H:\luma\payloads\CthulhuHomeOSD182.firm`;
- size 341504 bytes;
- SHA-256
  `9FA0BDD976FF098E3B0A7EBAD263DEB44B40B8F163F7409034E8BC9F1D9D0FDD`;
- tested V175 was archived recoverably as
  `H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD175-preV182.firm`;
- protected root `H:\boot.firm` remains unchanged at SHA-256
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Next device iteration: chainload V182 and verify the visible V182 label, initial
L+Y overlay, post-Notifications L+Y overlay, and power off. Then perform one
opposite-direction sort. The sort is still expected to commit persistently and
require reboot; the live icons are not expected to move yet. The important new
result is that the diagnostic completes without a crash and writes
`layout-backrefs-v182.txt`. Reinsert the card afterward. If the bounded scan again
finds zero raw/grid pointer references, stop repeating broad pointer scans and
move to HOME-side instrumentation of the confirmed layout event/object path.

### 2026-08-10 — V183 restores controller input from the proven live HID ring

The V182 device test found a specific post-Notifications regression: L+Y still
opened the panel, but A/B/X/Y and directional input did not operate it. The
initial interpretation incorrectly elevated V175's deliberately incomplete
recovery design note over the later user-confirmed baseline recorded at the
start of live-sort work: the overlay/menu worked both before and after
Notifications. Treat that later observed behavior as the required baseline.

The existing SD logs isolated two diverging input paths. The independent probe
continued to record HID transitions and chord edges, while the interactive
runtime snapshot reported held input zero and HOME input suppression remained
active. Therefore rendering, the HOME hook, and physical input were alive; the
controller's libctru `hidScanInput()`/`hidKeysHeld()` state was stale after the
applet transition. The L+Y recovery path worked because the independent probe
already read the official HID shared-memory ring directly.

V183 makes one functional correction in canonical and build-tree
`cthulhu_runtime_logger.c`: new helper `readLiveHidHeld` reads the current ring
index at HID shared memory `+0x10` and the corresponding held word at
`+0x28 + index*0x10`. Both the existing probe and the full interactive
controller now use this same helper. The controller no longer calls
`hidScanInput()` or `hidKeysHeld()`. Button-edge calculation, repeat behavior,
selection state, redraw, sorting calls, HOME input suppression, panel rendering,
and persistence code are unchanged.

Diagnostics identify the iteration as runtime/watchdog 1.8.3 with mode
`live-hid-ring-v183`; the watchdog path is now
`/3ds/Cthulhu/watchdog-v183.txt`. The visible Rosalina label is
`Check HOME OSD V167 / Input V183`. The V182 safe back-reference scan remains
present and unchanged.

Safety/build/deployment:

- the build succeeded in the existing Docker toolchain;
- HOME injection symbols remain the exact proven footprint: start
  `0x14007024`, frame `0x1400722C`, end `0x140075F4`;
- sole active payload is
  `H:\luma\payloads\CthulhuHomeOSD183.firm`, size 341504 bytes, SHA-256
  `82B21ED9BC156326DC1E04FA10D676B6B151EC2E7EADA4B69152B523BB20CF0A`;
- V182 is archived recoverably as
  `H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD182-preV183.firm`;
- V175 and exact V167 remain older rollback choices in the history directory;
- protected root `H:\boot.firm` remains unchanged at SHA-256
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Exact test sequence: boot V183 and verify its visible label. Open L+Y before
Notifications and exercise Up/Down, Left/Right, and B without applying a sort.
Open Notifications, return HOME, wait two seconds, open L+Y, and exercise the
same buttons. HOME icons must not navigate while the panel owns input. Then
close the panel and verify power off. Do not test sorting in this input-only
iteration. If post-Notifications input still fails, reinsert the SD and inspect
`watchdog-v183.txt` plus `framework-live-v167.txt`; do not change hooks or
rendering, because the remaining discriminator is controller attachment to the
active HOME PID rather than HID transport.

### 2026-08-10 — V184 migrates the full controller to the active HOME PID

V183's post-Notifications buttons still failed. Its automatically collected
logs conclusively separated transport from ownership: the watchdog identified
PID 40 as active with an advancing heartbeat, while the full interactive
runtime remained mapped to stalled PID 15. The independent probe recorded 53
transitions and four chord edges, and successfully toggled PID 40 twice. Thus
V183's direct HID reader worked; the controller was operating the wrong HOME
channel and panel.

V184 changes controller attachment only. A validated `openHomePid` helper opens
an exact PID and verifies the HOME title ID. The runtime prefers
`watchdogActiveHomePid`, records `runtimeAttachedPid`, and exits its mapped loop
when the watchdog selects a different active HOME. It then unmaps/closes the old
process and remaps the complete existing controller to the active process's
channel and panel. Selection, redraw, button repeat, sorting calls, input
suppression, persistence, and rendering remain the same controller code rather
than being partially duplicated in the recovery probe.

The L+Y probe fallback now runs only while the active PID differs from
`runtimeAttachedPid`. On a new attachment, the controller initializes its
previous-button word from the live HID ring; this prevents a held recovery chord
from being interpreted again and double-toggling the panel. The watchdog adds
`runtime_attached_pid` so future ownership failures are directly visible.

Diagnostics are runtime/watchdog 1.8.4, mode
`active-home-controller-v184`, log `/3ds/Cthulhu/watchdog-v184.txt`, and visible
label `Check HOME OSD V167 / Active V184`. V182's bounded post-sort diagnostic
remains unchanged but should not be invoked during this input test.

Build/deployment:

- build succeeded; exact HOME stub symbols remain `0x14007024`, `0x1400722C`,
  and `0x140075F4`;
- sole active payload is
  `H:\luma\payloads\CthulhuHomeOSD184.firm`, size 342016 bytes, SHA-256
  `42D1680891B234BB0D1937831E1B9F0DBAAD99FDF2C2D55B91639A1B48CCA6C8`;
- V183 is archived recoverably at
  `H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD183-preV184.firm`;
- protected `H:\boot.firm` remains unchanged at SHA-256
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Test V184 without sorting: verify the visible V184 label; exercise panel input
before Notifications; close it; open Notifications and return; wait two
seconds; use L+Y and test Up/Down, Left/Right, and B. Confirm HOME icons remain
suppressed only while the panel is open, then verify power off. If it fails,
reinsert the SD. `watchdog-v184.txt` must show whether
`runtime_attached_pid == active_home_pid`; that single comparison determines
whether attachment or controller behavior remains at fault.

### 2026-08-10 — V184 confirmed; V185 targets complete layout ownership

The user confirmed V184 is good: the complete panel controller is interactive
after returning from Notifications. This establishes V184 as the latest
confirmed rollback for overlay lifecycle, input ownership, and active-HOME
attachment. It was archived on the SD as
`H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD184-CONFIRMED.firm`.

Work returned to the remaining feature: changing visible icon order without a
reboot. Offline disassembly of the preserved USA HOME code established a more
precise native sequence. Publish routine `0x00146D10` has exactly one static
caller at `0x001BA830`. It is inside complete layout-event routine
`0x001BA594`, which for mode zero performs additional operations around the
already-tested rebuild `0x0013C680` and publish `0x00146D10`. Its confirmed
global literals are wrapper `0x003827D8` and rebuild subobject `0x003827E4`.
Static callers of the complete layout event are `0x001B9570`, `0x001B9F54`,
and `0x001B9FB8`. Calling only rebuild/publish explains why serialized/grid
state changed while instantiated visible icons did not.

V185 does not call the complete event without its real owner. Instead it
upgrades the safe bounded V182 scan into a targeted ownership report. After one
successful sort, `ScanLiveLayoutOwnershipV185` scans readable HOME regions in
64 KiB chunks through fixed local window `0x00900000` for four exact targets:
the discovered raw layout, processed grid, wrapper `0x003827D8`, and rebuild
subobject `0x003827E4`. Unlike V182 it includes readable code, so the report
captures known static literals/call neighborhoods and any heap/runtime owner
references together. It remains read-only, caps matches at 96, uses the static
12 KiB report buffer, and unmaps every chunk. Output is
`/3ds/Cthulhu/layout-ownership-v185.txt`.

No HOME assembly hook, controller behavior, rendering, sort mutation,
persistence, folder logic, or native call changed. The visible label is
`Check HOME OSD V167 / Owner V185`. Build succeeded with exact hook symbols
`0x14007024`, `0x1400722C`, `0x140075F4`.

Deployment:

- sole active payload `H:\luma\payloads\CthulhuHomeOSD185.firm`;
- size 342016 bytes;
- SHA-256
  `D22E6DC4BDFA2FC4C58AF1ACA67B828DC3EA2E5FE1ACB2A8CFD87D2EAA7266F4`;
- protected `H:\boot.firm` unchanged at
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Test once: boot V185 and verify the label. Before opening Notifications, open
L+Y, select the visibly opposite direction, and press A. Allow the ownership
scan to finish; live icon movement is not yet expected. Close the panel, verify
power off, and reinsert the SD. Read `layout-ownership-v185.txt` automatically.
References outside known code literals provide the owner candidate for a later
guarded complete-event call. If only code literals appear, use their natural
caller chain for HOME-side object capture rather than another broad scan. Any
crash or power-off regression requires immediate rollback to confirmed V184.

### 2026-08-11 — V185 found live wrapper references; V186 traces their owners

The V185 test completed safely. Persistent sorting committed with result zero,
native request/ack 1, and native result zero. No new crash dump was created;
the two dumps on the card predate V185. As expected, visible icon refresh was
not yet claimed.

`layout-ownership-v185.txt` scanned 15 readable regions and 5,978,112 words. It
again found zero direct raw/grid references and zero direct references to
rebuild subobject `0x003827E4`, but found six exact live references to wrapper
`0x003827D8`:

- heap/object locations `0x0800FCBC`, `0x08018B7C`, and `0x08032A44`;
- stack locations `0x0FFFF5F4` and `0x0FFFF60C`;
- layout allocation location `0x3467A684`.

The last reference is especially concrete: the surrounding allocation contains
other pointers into HOME's serialized layout area. The `0x080...` references
are plausible runtime objects, while the stack references can reveal the active
call chain. This is the first positive ownership evidence after V176-V182 and
justifies a bounded second-level scan rather than another native call guess.

V186 retains the same first pass and records up to 16 wrapper-reference
addresses. It then performs one additional read-only pass through the same
bounded readable regions, searching for pointers to those exact reference
locations. Each match records the owner address, target address, and adjacent
words in `/3ds/Cthulhu/layout-ownership-v186.txt`. This distinguishes heap
objects that own/reference the wrapper-bearing subobjects and captures stack or
manager links. It caps second-level results at 64, continues using the static
12 KiB report buffer and fixed 64 KiB mapping window, and unmaps every chunk.

No HOME hook, controller, rendering, sorting mutation, folder behavior,
persistence, or native refresh call changed. Visible label is
`Check HOME OSD V167 / Owner V186`. Build succeeded with the exact confirmed
stub symbols `0x14007024`, `0x1400722C`, `0x140075F4`.

Deployment:

- sole active `H:\luma\payloads\CthulhuHomeOSD186.firm`;
- size 342528 bytes;
- SHA-256
  `BB320CFE7B8A1A712DFCEB77B5EB0D8D206CC29C50511B3D5EE5FF314AB0447E`;
- V185 archived as
  `H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD185-SUCCESS.firm`;
- confirmed V184 remains archived separately;
- protected `H:\boot.firm` remains unchanged at
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Test one opposite-direction sort before Notifications, let the two-pass scan
finish, close the panel, verify power off, and reinsert. Live icon movement is
not expected. Read `layout-ownership-v186.txt`; non-stack `owner_ref` results
identify the next signature-validated HOME-side capture point. Zero owner refs
means inspect the wider first-level object windows directly rather than adding
another broad scan. Roll back to confirmed V184 on any safety regression.

### 2026-08-11 — V186 identifies bound icon callback; V187 attempts live refresh

V186 completed safely, persistent sort committed with result zero, and the
ARM11 dump directory remained empty after the old dumps were deliberately
removed before the test. It found seven wrapper-bearing runtime records and
zero second-level pointers to those record addresses. This means the records
are likely callback/context storage rather than conventional owned subobjects.

Three records placed executable HOME addresses directly beside wrapper
`0x003827D8`: `0x001B34A4`, `0x001CA504`, and `0x001BC9B0`. Offline disassembly
shows `0x001CA504` is the relevant icon refresh callback. It accepts its object
in `r0`, walks all 360 icon slots, examines each icon's live model state, and
invokes HOME's per-icon update path. Its runtime callback record at
`0x08032A3C` stores function `0x001CA504`, then padding, then bound context
`0x003827D8` eight bytes later. This supplies both native function and argument
from live HOME memory rather than inference from similar static functions.

V187 is the first guarded live-refresh attempt. After the already-proven
frame-bound rebuild `0x0013C680` and publish `0x00146D10`, the HOME stub loads
the existing publish owner `0x003827D8` and calls `0x001CA504` exactly once per
sort request. It records the owner and call count at channel `+0xF8/+0xFC`,
which `framework-live-v167.txt` now reports as `icon_refresh_owner` and
`icon_refresh_calls`. The V186 scanner is retained in source but intentionally
unused; no broad scan runs during V187 sorting.

The complete V184 active-PID controller, Notifications recovery, direct HID
input, input suppression, rendering, persistent mutation, folder logic,
backup/readback, and graceful shutdown transaction are unchanged. This new
native call has higher risk than read-only V186. Mitigations are exact callback
and bound-context evidence, execution only at HOME's proven frame boundary,
one call per acknowledged transaction, preserved caller registers, an empty
crash-dump directory, and recoverable V184/V186 payloads.

Version/build/deployment:

- visible label `Check HOME OSD V167 / Live V187`;
- stub start/frame unchanged at `0x14007024`/`0x1400722C`; end grows only to
  `0x1400761C`, well below cave limit `0x14007F00`;
- sole active payload `H:\luma\payloads\CthulhuHomeOSD187.firm`;
- size 340992 bytes;
- SHA-256
  `415B3853A759EDA01DF88021BA4F78F48DD67631833CED7DF684FA9636028C43`;
- V186 archived as
  `H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD186-SUCCESS.firm`;
- confirmed V184 remains available;
- protected root `H:\boot.firm` unchanged at
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Test conservatively: boot V187 and verify its label, open the panel before
Notifications, select the visibly opposite direction, and press A once. Watch
whether icons reorder immediately. If HOME crashes or becomes unstable, power
off rather than retrying; the clean dump directory makes any dump attributable
to V187. If it returns normally, close/reopen the panel, verify its input, and
power off. Reinsert the SD either way. Inspect `framework-live-v167.txt` for
owner `003827D8` and call count 1. A call count of 1 with no movement means the
callback executed but a subsequent model-publication stage remains; a crash
requires immediate V186 or confirmed V184 rollback and exact dump decoding.

### 2026-08-11 — V187 crashed from wrong callback context; V188 observes real ABI

V187 crashed immediately when applying a sort. The clean dump directory made
the new file unambiguous. It was preserved locally as
`runtime/.analysis/crash-v187-native-icon-refresh.dmp`, size 332 bytes,
SHA-256 `708F420F9816EBB17A526D9F64B9DDFB57AF0045EF566BDF402A3A12B1BB9434`.
V187 was moved to
`H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD187-CRASHED.firm`
and successful V186 was restored before further work.

Exact dump decoding:

- ARM11 core 0, data abort in HOME process `menu`;
- PC `0x001CA524`, inside the proposed 360-icon callback;
- LR `0x003053FC`, inside the injected frame hook;
- `r5=0x003827D8`, proving the supplied context reached the function;
- the callback loaded `[r5+0x128] = 0x000398F8` and then faulted while
  dereferencing it at `0x001CA524`;
- FAR/r0 `0x000398F8`, DFSR status 5 translation fault;
- journal stopped at `atomic-home-unlocked`, and the icon-refresh counter
  remained zero because the native call did not return.

Conclusion: function `0x001CA504` remains a real 360-icon model pass, but the
adjacent `0x003827D8` value in the discovered runtime record is not that
function's direct `this` pointer. The V187 interpretation of the callback
record ABI was wrong. Do not repeat the direct call with this context.

V188 removes the unsafe direct call completely. It adds a signature-gated,
logging-only branch at native entry `0x001CA504`, requiring original prologue
`0xE92D5FF0`. `cthulhuIconRefreshObserveHook` records natural incoming `r0` and
increments a count at channel `+0xF8/+0xFC`, restores temporary registers,
replays the exact displaced `stmfd sp!,{r4-r11,r12,lr}`, and resumes at
`0x001CA508`. It never invokes the callback, changes arguments, or writes the
object. Existing framework logging already exposes these fields as
`icon_refresh_owner` and `icon_refresh_calls`.

All V184/V186 controller, Notifications recovery, input suppression, sorting,
folder, persistence, and power-off behavior is otherwise unchanged. The
preserved dump was removed from the SD after local archival, leaving the ARM11
dump directory empty again.

Build/deployment:

- visible label `Check HOME OSD V167 / Observe V188`;
- observer loader symbol `0x14007414`; stub start/frame/end
  `0x14007024`/`0x1400722C`/`0x1400761C`;
- sole active `H:\luma\payloads\CthulhuHomeOSD188.firm`;
- size 340992 bytes;
- SHA-256
  `89ED3A44B4313C15771B044B7BB1C45A07160C99FEF0E3A1870D5F6239B2049F`;
- successful V186 remains archived, including a post-V187 rollback copy;
- protected root `H:\boot.firm` unchanged at
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

V188 test must not sort. Boot and verify the label, move across several HOME
pages, open and close one folder, wait two seconds, verify L+Y and power off,
then reinsert. Read `framework-live-v167.txt`. A nonzero
`icon_refresh_owner` with positive calls supplies the real ABI object for a
later guarded call. Zero means this callback is only dispatched through a
specific event path; do not call it manually again without tracing the indirect
dispatcher and context transform.

### 2026-08-11 — V188 natural observer stayed zero; V189 scans icon class table

V188 booted, navigated pages/folder, displayed the overlay, and powered off
without a crash; the clean ARM11 dump directory stayed empty. Its automatic log
reported `icon_refresh_owner=0` and `icon_refresh_calls=0`. Therefore HOME does
not naturally enter `0x001CA504` during ordinary page/folder navigation. The
routine is dispatched only for a more specific event, and the observer did not
provide its ABI object.

Offline binary reference analysis found the only literal `0x001CA504` at
`0x0030AC88`, the final slot of a related icon-model function table beginning
around `0x0030AC50/0x0030AC58`. Adjacent entries include `0x001C9CD4`,
`0x00237E64`, `0x001FDF2C`, `0x001C8D98`, `0x001CA4B8`, and `0x001C8910`.
This changes the interpretation of V186's `0x08032A3C` record: it contains a
copied/selected table function plus other dispatch metadata, not a directly
callable function/context pair. A live object should instead contain a pointer
to the class table or a copied function pointer at a stable object offset.

V189 keeps V188's logging-only entry observer and re-enables the safe post-sort
bounded scan with exact additional targets `0x0030AC50`, `0x0030AC58`,
`0x0030AC60`, and `0x001CA504`. It scans readable HOME regions through the
fixed 64 KiB local window, logs surrounding words for class/function matches,
and retains the prior wrapper/raw/grid evidence in
`/3ds/Cthulhu/icon-class-v189.txt`. It makes no native refresh call. The sort
path is the already-proven persistent transaction; live movement is not
expected.

Build/deployment:

- visible label `Check HOME OSD V167 / Class V189`;
- safe observer/stub symbols unchanged at `0x14007414`, start `0x14007024`,
  frame `0x1400722C`, end `0x1400761C`;
- sole active `H:\luma\payloads\CthulhuHomeOSD189.firm`;
- size 342528 bytes;
- SHA-256
  `C7610490971A91D6EE04E1F122764453FE93EA288E9DEE404E17C48D2D5D471B`;
- V188 archived as
  `H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD188-SAFE.firm`;
- successful V186 and confirmed V184 remain available;
- protected root firmware unchanged at its established hash.

Test one opposite-direction sort before Notifications, allow the read-only scan
to finish, verify power off, and reinsert. Read `icon-class-v189.txt`. A class
pointer in writable heap gives the real object base directly; copied function
matches with consistent surrounding object pointers identify the dispatcher
record layout. Any crash must be treated as V189 and rolled back to V188/V186,
though no new native function is invoked in this iteration.

### 2026-08-11 — V189 proves actual object layout; V190 retries with validated owner

V189 completed safely, persistent sorting committed with result zero, and the
clean crash directory remained empty. The class scan found one exact live
`0x001CA504` slot at `0x08032A38` and no direct class-table pointers. Its
surrounding words establish the full object layout:

- object base `0x08032918`;
- callback slot `object+0x120 = 0x001CA504`;
- callback-required model field `object+0x128 = 0x34635E00`, a valid HOME heap
  pointer;
- wrapper field `object+0x12C = 0x003827D8`.

This exactly explains V187. V187 passed the wrapper field at `+0x12C` as if it
were the object base, so the callback's `[r0+0x128]` load read unrelated global
value `0x000398F8` and faulted. The correct ABI object is dynamically derived as
the live function-slot address minus `0x120`; it must not be hard-coded because
the `0x080...` heap location may change across boots.

V190 upgrades `ScanLiveIconClassV190` to derive this owner only when both
conditions hold: an exact function slot `0x001CA504` exists in readable HOME
memory, and its `+8` word (object `+0x128`) is a plausible HOME pointer in
`0x08000000..0x3FFFFFFF`. The report now writes
`/3ds/Cthulhu/icon-class-v190.txt` with `icon_owner` and `icon_model`. Rosalina
publishes validated owner to channel `+0x100`; zero causes the HOME call to be
skipped.

At the existing acknowledged frame-bound rebuild sequence, HOME loads only that
validated dynamic owner and calls `0x001CA504` once. The retained entry observer
records actual incoming owner/call count at `+0xF8/+0xFC`; completion is recorded
at `+0x104`. Framework logging exposes requested owner and completion separately.
No wrapper address is used as the callback object.

This remains a higher-risk native-call iteration, but unlike V187 both the
function and ABI object layout are now proven from the same live object. Existing
controller, Notifications recovery, input suppression, rendering, persistent
sort, folder, backup/readback, and power-off logic are unchanged. The empty dump
directory makes any crash attributable to V190.

Build/deployment:

- visible label `Check HOME OSD V167 / Live V190`;
- stub start/frame/observer/end `0x14007024`, `0x1400722C`, `0x14007438`,
  `0x14007640`, still below the cave limit;
- sole active `H:\luma\payloads\CthulhuHomeOSD190.firm`;
- size 342528 bytes;
- SHA-256
  `C1769CDCD7804A01C8B5032F1F906651C2E5C011EB9AF30A7402AB750D146FAA`;
- V189 archived as
  `H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD189-SUCCESS.firm`;
- V188/V186/V184 remain available;
- protected root firmware remains unchanged.

Test conservatively: verify the V190 label and apply one visibly opposite sort
before Notifications. Watch for immediate icon movement. If it crashes or HOME
becomes unstable, power off without retrying and reinsert. If normal, close the
panel, verify input and power off, then reinsert. Inspect the V190 report and
framework fields: requested owner, observed owner, observer call count, and
completion must agree. Completion 1 with no movement means this pass updates
per-icon state but a later draw/rebind stage remains; a crash requires exact
dump decoding and immediate V189 rollback.

### 2026-08-11 — V190 callback completed with correct ABI; visual result pending

V190 completed without a crash and the ARM11 dump directory remained empty.
The persistent sort journal reached `shutdown-sort-committed` with result zero.
All independent diagnostic fields agree:

- scan-derived owner `0x08032918`;
- validated model pointer `0x34635E00`;
- requested owner `0x08032918`;
- observer-recorded actual native owner `0x08032918`;
- observer call count 1;
- post-return completion count 1;
- rebuild request/ack/calls all 1 with native result zero.

Therefore the dynamically derived ABI object is correct, the native
`0x001CA504` call returned normally, and V187's crash was specifically caused
by passing wrapper field `object+0x12C` instead of the object base. V190 is safe
on this test and must remain the active payload until the user's visual result
is recorded.

The logs cannot determine whether visible icons actually changed position.
That observation is the decision boundary: immediate visible reorder means
V190 is the first working no-reboot sort; no movement means this 360-entry pass
updates icon state but not the instantiated position binding, and the next work
must trace the later draw/rebind consumer rather than call this routine again.
Do not alter or redeploy until the visual outcome is added.

### 2026-08-11 — V190 safe but icons did not visibly reorder

The user confirmed the visual result: V190 did not crash, but the icons did not
move. Combined with the exact counters above, this proves `0x001CA504` is a
valid 360-entry model-state pass with the correct object ABI, but it is not the
visible position rebind/publish boundary. Do not repeat or multiply this call;
it already returned successfully once and its effect was insufficient.

V190 remains the active safe payload. Next work must trace consumers of the
dirty/model state produced by this object, especially related methods in the
`0x0030AC58` function table and the dispatcher/draw stage that turns model
positions into instantiated icon transforms. Preserve the dynamically derived
owner and completed call as useful evidence, but do not add another speculative
native call until the subsequent consumer and its ABI are identified.

### 2026-08-11 — V191 compares rebuilt grid with separate instantiated model

Further disassembly explains V190's safe visual no-op. Callback `0x001CA504`
does not consume the processed grid at `0x346CCF90`. It loads
`owner+0x128 = 0x34635E00`, then reads a record-array pointer from
`iconModel+0x398F8`, and iterates 360/420 records of stride `0x230`. It updates
per-icon state from that separate instantiated model. Since the persistent sort
and native rebuild/publish did not synchronize this model first, V190 correctly
refreshed from the old icon positions.

The missing boundary is therefore model synchronization, not another redraw.
V191 is read-only and disables the V190 callback request by publishing zero to
channel `+0x100`. Its validated owner scan additionally maps
`iconModel+0x398F8`, records the live record-array pointer, and logs the first
12 records (selected words including fields used by the callback) beside the
first 12 rebuilt processed-grid title IDs. Output is
`/3ds/Cthulhu/icon-model-v191.txt`. Mapping is bounded to one pointer page and
at most four record pages through the established fixed local window; all maps
are removed before HOME resumes.

No new native function is called. Existing persistent sorting, controller,
Notifications recovery, input suppression, folder behavior, backup/readback,
and power-off logic are unchanged. Live movement is not expected in V191.

Build/deployment:

- visible label `Check HOME OSD V167 / Model V191`;
- HOME stub symbols unchanged from safe V190: start `0x14007024`, frame
  `0x1400722C`, observer `0x14007438`, end `0x14007640`;
- sole active `H:\luma\payloads\CthulhuHomeOSD191.firm`;
- size 343040 bytes;
- SHA-256
  `31807DBF97C42537F16EFB016C06AE0E829A2C2C9B4DF4750102A8C6E7ADDE0B`;
- safe V190 archived as
  `H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD190-SAFE-NOMOVE.firm`;
- V189/V188/V186/V184 remain available;
- crash directory empty and protected root firmware unchanged.

Test one visibly opposite sort before Notifications, let the read-only model
comparison finish, verify power off, and reinsert. Icons are not expected to
move. The report's record fields versus processed-grid IDs will identify
whether the model stores title IDs directly or uses indices/handles, determining
whether synchronization can be a bounded data update or must invoke its native
builder.

### 2026-08-11 — V191 proves title-ID records; V192 matches sorted SD titles

V191 completed safely, committed the persistent sort, and left the crash
directory empty. It resolved the live record array to `0x34636038`. Each record
is `0x230` bytes and begins with a title ID split low/high across words 0/1.
For example record 1 is title `0004001000021300`. The initial records are
NAND/system titles, while the rebuilt processed grid is SD-only and its first
positions are empty, so the initial index-by-index comparison was intentionally
not interpreted as a position mapping.

V192 performs the decisive read-only join. It maps the bounded full 420-record
array (at most `0x3B000` bytes), walks nonempty entries in `g_sortGrid`, matches
each title ID against record words 0/1, and logs up to 24 matches with grid
index, record index, and record words 2-7 and 14. These include the packed fields
consumed by the icon callback and reveal whether grid position/folder is stored
directly or represented through indices/handles. Output is
`/3ds/Cthulhu/icon-model-v192.txt`.

V190's callback remains disabled (`channel+0x100=0`), and V192 makes no new
native call or model write. Existing persistent sorter, controller,
Notifications recovery, input suppression, folder, backup/readback, and
shutdown behavior are unchanged. Live movement is not expected.

Build/deployment:

- visible label `Check HOME OSD V167 / Model V192`;
- unchanged HOME symbols: start `0x14007024`, frame `0x1400722C`, observer
  `0x14007438`, end `0x14007640`;
- sole active `H:\luma\payloads\CthulhuHomeOSD192.firm`;
- size 343552 bytes;
- SHA-256
  `525863189641B1269BDEE18AF8D315A0C0BB499DD0282B22C92273685677D731`;
- V191 archived as
  `H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD191-SUCCESS.firm`;
- safe V190 and older rollbacks remain available;
- crash directory empty and protected root firmware unchanged.

Test one visibly opposite sort before Notifications, allow the read-only match
report to finish, verify power off, and reinsert. Read `icon-model-v192.txt`.
Consistent record fields correlated with grid index authorize a narrowly bounded
model update plus the already-safe V190 callback; absence of correlation means
trace the native model builder instead of writing records directly.

### 2026-08-11 — V192 identifies position indirection; V193 captures the map

V192 completed safely, committed the persistent sort, and left the crash
directory empty. Its first 24 nonempty sorted-grid title IDs matched live icon
records exactly, but at record indices 183 down through 160 while grid positions
were 15 through 38. The sampled record fields did not contain a changing grid
position. This proves the `0x230`-byte record array is an object store rather
than an array that should be reordered wholesale.

Static ARM disassembly then identified HOME's normal lookup boundary. Multiple
UI accessors translate logical positions through signed 16-bit indices at
`iconModel+0x4450E`, while related accessors load an adjacent pointer from
`iconModel+0x44508` before indexing the same `0x230`-byte records. This explains
the V190 result: its validated `0x001CA504` callback refreshed the existing live
map after the serialized SD grid changed, so it correctly redrew the old order.
Do not memcpy or swap full records; they contain live object state and pointers.

V193 remains read-only. It preserves the complete V192 report, then maps one
page around `iconModel+0x44500`, logs four header words, 60 signed inline values
starting at `+0x4450E`, and, when the `+0x44508` word is a valid HOME address,
maps and logs 60 signed values from that indirect array. Output is
`/3ds/Cthulhu/icon-model-v193.txt`. No native function is called and the V190
callback remains disabled (`channel+0x100=0`). Existing persistent sorting,
controller, Notifications recovery, input suppression, folders, backup/readback,
and shutdown behavior are unchanged. Live movement is not expected in V193.

Build/deployment:

- visible label `Check HOME OSD V167 / Map V193`;
- sole active `H:\luma\payloads\CthulhuHomeOSD193.firm`;
- size 344064 bytes;
- SHA-256
  `E82380B3C671858959F4376C9D9809C5504BAE7FB5121B208746F0C2EF449847`;
- V192 archived as
  `H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD192-SUCCESS.firm`;
- crash directory empty;
- protected `H:\boot.firm` remains unchanged at SHA-256
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Test one visibly opposite sort and allow the report to finish, verify overlay
input and power off, then reinsert the card. Compare the inline and indirect
indices with the V192 grid-to-record matches. The active table should contain
the old record sequence; the following iteration should trace or invoke the
native routine that rebuilds only that map, then use the already-safe V190
refresh callback.

### 2026-08-11 — V193 confirms two stale live maps; V194 inverts both

V193 completed safely and produced `icon-model-v193.txt`; the crash directory
remained empty and the persistent sort journal reached
`shutdown-sort-committed` with result zero. The live model was again
`0x34635E00`, its record array was `0x34636038`, and the V192 title join was
reproduced: sorted grid positions 15 through 38 correspond to records 183 down
through 160.

The map header at `iconModel+0x44500` was
`FFFFFFFF,0000FFFF,3466F6FE,0001FFFF`. The word at `+0x44508` is a valid but
halfword-aligned HOME address (`0x3466F6FE`). The inline signed-16 map at
`+0x4450E` begins `1,0,2,...,13,166,16,...`; the indirect map begins
`1,0,2,...,13,187,360,15,16,...`. Neither can be overwritten from grid position
alone because they clearly include distinct special/system translations.
This confirms stale live indirection but not yet which entries represent the SD
layout subset.

V194 is another bounded read-only join. It retains both 60-entry samples, then
scans all 420 halfwords in each already-mapped page for every one of the 24
matched SD record indices. For each table and matched title it logs the first
map position and occurrence count. This inversion will identify the exact
contiguous or sparse SD subset while preserving special entries. Output is
`/3ds/Cthulhu/icon-model-v194.txt`. No HOME memory is written, no new native
function is called, and the insufficient V190 callback remains disabled.

Build/deployment:

- visible label `Check HOME OSD V167 / Map V194`;
- sole active `H:\luma\payloads\CthulhuHomeOSD194.firm`;
- size 344064 bytes;
- SHA-256
  `D196AA8B29EAFA9C7D38FC265D7B4D8A69C6FADEA421E7A6B83B0793B8AC4265`;
- V193 archived as
  `H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD193-SUCCESS.firm`;
- protected root firmware remains unchanged at SHA-256
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Test one opposite sort, verify overlay input and power off, then reinsert. Read
the `inline_match` and `indirect_match` lines. Only after the active SD subset
is proven should the next version update those signed indices and request the
validated V190 refresh callback.

### 2026-08-11 — V194 isolates membership; V195 guarded live reorder

V194 completed safely, left no crash dump, and again committed the persistent
sort with result zero. Its inversion proves that both maps contain the SD record
IDs but at stale and different positions. In the inline map, most matched
records 160-183 occur once around positions 159-179, with record 166 at special
position 14 and two records absent. In the indirect map, all 24 occur exactly
once but at sparse positions 161-201. Directly copying grid indices into either
map would therefore overwrite special entries and remains forbidden.

V195 performs the first guarded live mutation derived from these results. While
HOME is still suspended after the successful persistent transaction, it joins
every nonempty processed-grid title ID against all 420 live records to build the
desired record-ID sequence. For each map independently it then:

1. records only positions whose current value is one of those recognized SD
   title records;
2. filters the desired sorted sequence to the exact membership already present
   in that map;
3. refuses the write unless position and ordered-member counts match and contain
   at least two entries;
4. permutes only those signed-16 values in place, leaving all unknown, system,
   folder/special, gap, and absent-title values untouched;
5. flushes both local and HOME process caches for the one mapped page.

The inline and indirect maps are handled separately, preserving top-level versus
broader membership. The report logs desired/missing counts plus positions,
ordered counts, and changed counts to
`/3ds/Cthulhu/icon-model-v195.txt`. After HOME resumes, V195 requests the
previously validated `0x001CA504` refresh through owner `0x08032918`; this is the
same owner/call combination that completed safely in V190. Existing hook,
Notifications recovery, controller, suppression, persistent folder handling,
backup/readback, and power-off behavior are unchanged.

Build/deployment:

- visible label `Check HOME OSD V167 / Live V195`;
- sole active `H:\luma\payloads\CthulhuHomeOSD195.firm`;
- size 345088 bytes;
- SHA-256
  `AA1359D0D8FBEE85407452D557E99967FC9F00764BA08CE34396DE1A7EAE325E`;
- V194 archived as
  `H:\luma\disabled\CthulhuFrameworkHistory\CthulhuHomeOSD194-SUCCESS.firm`;
- safe V190 and confirmed V184 remain available;
- protected root firmware remains unchanged at SHA-256
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Test a visibly opposite sort. Observe whether icons move immediately, whether
the overlay stays responsive, and whether power off works. If HOME crashes,
power off and reinsert without retrying; preserve the dump and restore V194 or
V190. If it remains stable, reinsert and inspect V195 changed counts and callback
telemetry before expanding live handling to folders.

### 2026-08-11 — Working V195 baseline published as LumaHome

The user confirmed V195 appeared to sort live successfully and requested that
the modified firmware become a separate project rather than remain embedded
only in Cthulhu. A GitHub fork was created as
`https://github.com/TheMikaus/LumaHome`. GitHub records it as a fork of
`LumaTeam/Luma3DS`; its base remains upstream commit `d30ac8d` (after v13.4),
and the GPLv3 license/history are preserved.

The existing full Luma working tree at `runtime/.build/Luma3DS` is itself a
nested Git repository. Its remotes are now:

- `origin`: `https://github.com/TheMikaus/LumaHome.git`
- `upstream`: `https://github.com/LumaTeam/Luma3DS.git`

The complete V195 source baseline was committed there on branch
`lumahome/home-menu-framework` as commit
`ea91c96cf4fae5775260ca5013b213244385e25b` (`Add LumaHome HOME Menu framework
baseline`) and pushed to origin. The commit contains 14 intentional source and
documentation files (5,230 inserted lines); generated `.firm`, `.elf`, dumps,
and captures remain ignored. `LUMAHOME.md` documents the V195 feature status,
chainload safety, build method, upstream attribution, and the intentionally
retained transitional `cthulhu_` internal identifiers.

The exact committed standalone tree was rebuilt successfully with the existing
Docker/devkitARM toolchain after commit and before push. No pull request was
opened because this is the fork's own development baseline rather than a change
proposed to upstream Luma3DS. The original Cthulhu branch and SD rollback payloads
remain intact. Future HOME runtime development should occur in LumaHome; retain
Cthulhu only as an optional companion/configuration project until the split is
fully verified and historical build sources can be removed safely.

### 2026-08-11 — LumaHome 0.1.0-rc1 branded test candidate installed

At the user's request, the working V195 behavior was prepared as the first
LumaHome release candidate. User-facing branding now reads
`LUMAHOME 0.1 RC1`; the Rosalina status item reads
`LumaHome 0.1.0-rc1 status`; active runtime/map report headers identify
LumaHome and release `0.1.0-rc1`. The overlay success message now says
`LIVE MAP UPDATED / REBOOT NOT NEEDED` instead of the obsolete power-off
instruction. Active reports are written to `/3ds/LumaHome/runtime.txt` and
`/3ds/LumaHome/live-map-0.1.0-rc1.txt`.

ABI-sensitive internal `cthulhu_` symbols and compatibility storage paths were
intentionally not renamed. Persistent sorter inputs, backups, journal, and
transaction files still use `/3ds/Cthulhu`; changing those in the branding pass
would risk the already-validated transaction and hook ABI. The standalone docs
state this explicitly.

Standalone LumaHome commits pushed to `lumahome/home-menu-framework`:

- `cdfec73c508486f90adcc23089b7c1944076b89f` — brand V195 as
  LumaHome 0.1.0-rc1;
- `46b902a` — add the complete 11-section hardware test plan in
  `TEST-PLAN.md`.

The renamed source rebuilt successfully. SD installation:

- active test payload: `H:\luma\payloads\LumaHome010RC1.firm`;
- size 345088 bytes;
- SHA-256
  `C51EA51FF5A7506AB073097BF87E73E5C00463676B2BA04A6D64770ED70FCC59`;
- active payload directory now contains only `GodMode9.firm` and the LumaHome
  candidate; five legacy Cthulhu diagnostic/OSD payloads were moved to the
  existing disabled history directory;
- `/3ds/LumaHome` was created for active reports;
- protected `H:\boot.firm` remains unchanged at SHA-256
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Have the user execute `TEST-PLAN.md` sections 1-10 in one session, then power
off and reinsert once for section 11 evidence collection. Do not merge into
master or tag a release until the live-map/runtime logs, compatibility journals,
and crash directory have been inspected.

### 2026-08-12 — RC1 folder-membership failure; RC2 safety guard installed

The user stopped the RC1 regression at folder testing. After adding several
icons to a folder, applying the sort caused the wrong visible titles to move;
folder Before/After did not update live. Opening Notifications and returning to
HOME caused the folder to appear in its correct persisted location. No crash
dump was produced and the persistent transaction completed successfully.

RC1 evidence proves this is a stale live-model problem, not a persistent-layout
failure. The transaction contained 173 SD title records, including five folder
children. The live inline/top-level map still supplied 168 members and RC1
rewrote all 168; the indirect map rewrote 171 of 173. Its guard compared each
map's old membership with its own filtered desired list, so equal counts did not
prove that HOME's top-level membership matched the newly edited folder layout.
Notifications triggered HOME's natural model reload, after which the persisted
folder position became visible. Folder objects are special live records rather
than title-ID records and were intentionally skipped by V195/RC1, explaining
why Before/After itself did not move live.

LumaHome RC2 commit `d3b259e` adds a conservative membership guard. It records
all folder-contained record IDs separately and scans the inline/top-level map
for them. If any are present, HOME's model is stale: RC2 performs no inline or
indirect live permutation and does not request the native icon-refresh callback.
The report logs `folder_records`, `inline_stale_folder_members`, and
`live_update_deferred`. If no stale folder member exists, the previously proven
membership-preserving title permutation remains enabled. This prevents RC1's
wrong-title movement while work continues on invoking HOME's natural model
reload directly. It does not claim live folder Before/After is fixed; that still
requires the model-reload boundary demonstrated by Notifications.

Build/deployment:

- visible title `LUMAHOME 0.1 RC2`;
- active payload `H:\luma\payloads\LumaHome010RC2.firm`;
- size 345088 bytes;
- SHA-256
  `BADCC0463133C38A0D90B10BDE29FFEFCA3BA3B984D07D1D4EF22AD9E6E29819`;
- RC1 archived as
  `H:\luma\disabled\CthulhuFrameworkHistory\LumaHome010RC1-FOLDER-MAP-STALE.firm`;
- root firmware unchanged at SHA-256
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`;
- RC2 pushed to `TheMikaus/LumaHome` branch
  `lumahome/home-menu-framework`.

Next test is intentionally narrow: boot RC2, verify branding, leave the current
folder membership as-is, request A-Z and Z-A, and confirm no wrong title moves.
If live update is deferred, open Notifications and return; confirm the persisted
folder placement/order becomes correct and hooks still work. Reinsert once to
inspect `live-map-0.1.0-rc2.txt`. Sleep/wake is explicitly not tested because
the current device is a 2DS and is not a blocker for this safety iteration.

### 2026-08-12 — RC2 guard succeeds; false failure isolated; RC3 source committed

RC2 correctly detected the stale HOME model: 5 folder-contained records existed,
3 were still present in the inline/top-level map, `inline_changed=0`,
`indirect_changed=0`, and `live_update_deferred=1`. No wrong title was moved,
the persistent Z-A transaction committed 173 titles and one folder successfully,
and no crash dump was produced.

The overlay nevertheless showed failure `0xFFFFFFE0` (`-32`). This was not a
sort failure. RC2 set the icon refresh owner to zero but still incremented the
HOME callback request and waited for an acknowledgement. Because the callback
was intentionally disabled, no acknowledgement arrived and the timeout replaced
the successful transaction result.

RC3 source commit `0476e85` fixes that control flow. Deferred operations skip
the callback request/wait entirely, return the successful persistent result, set
a shared deferred flag, and display `SAVED SAFELY / LIVE MODEL STALE / REOPEN
HOME`. Genuine callback/live updates retain the existing success path; genuine
transaction errors retain failure. RC3 branding and report path are
`LUMAHOME 0.1 RC3` and `/3ds/LumaHome/live-map-0.1.0-rc3.txt`.

The existing Docker build command was attempted twice, but the automatic
permission-approval review timed out on both attempts before tool output was
returned. The local `boot.firm` hash remained the RC2 hash, so RC3 was not
installed and RC2 remains active on the SD card. Do not copy or push RC3 until
the exact committed source builds successfully. Resume by rerunning the normal
Docker build, verify the RC3 strings/hash, archive RC2, install
`LumaHome010RC3.firm`, push commit `0476e85`, and test the deferred UI path.

### 2026-08-12 — RC3 build retry succeeded and candidate installed

The user reported that no approval prompt had appeared. Retrying the same
Docker/devkitARM build then completed normally. The output size changed from
RC2 and the final build succeeded through Rosalina link, sysmodule packaging,
and `boot.firm` creation.

Deployment:

- active payload `H:\luma\payloads\LumaHome010RC3.firm`;
- size 345600 bytes;
- SHA-256
  `BCE1EBF007255D351742DEE6C5E4E2A383DBE84DDA17597E9CA12E158F32BD74`;
- RC2 archived as
  `H:\luma\disabled\CthulhuFrameworkHistory\LumaHome010RC2-SAFE-DEFER-GUARD.firm`;
- active payload directory contains only GodMode9 and RC3;
- protected root firmware remains unchanged at SHA-256
  `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Standalone LumaHome commit `0476e85` was pushed to
`lumahome/home-menu-framework`. Test the same stale-folder scenario once. The
expected outcome is an immediate successful persistent transaction with the
deferred message, no wrong icon movement, no three-second callback timeout, and
correct folder/title positions after HOME's model reload through Notifications.

### 2026-08-12 — RC3 overcorrected; RC4 restores live title sorting

The user correctly objected that RC2/RC3 disabled the live icon sorting already
proven by V195 and regression sections 1-4. The folder safety response was too
broad: detecting any stale folder-contained title disabled the entire inline
permutation and therefore removed normal A-Z/Z-A live movement.

RC4 commit `65164b9` restores live sorting while isolating the stale subset. It
now derives separate record sequences for freshly persisted top-level titles
and folder-contained titles. For the inline/top-level map it:

- finds and leaves every stale folder-child record untouched;
- finds positions containing records that are still top-level;
- orders only those top-level members according to the fresh sorted grid;
- requires the selected position/member counts to match before writing;
- performs the validated refresh callback whenever those unaffected records
  changed.

The indirect map retains the V195 membership-preserving full-title permutation.
When stale folder children were skipped, the overlay reports
`LIVE SORT PARTIAL / FOLDER MODEL STALE / REOPEN HOME`; this is informational,
not failure. Normal sessions without stale membership retain the standard live
success state. Folder-object movement still waits for HOME's natural model
reload and remains the next architectural task.

Build/deployment:

- visible title `LUMAHOME 0.1 RC4`;
- active payload `H:\luma\payloads\LumaHome010RC4.firm`;
- size 345600 bytes;
- SHA-256
  `324A55EE4770DF96F0FA51F48BF928C491239281247C9A5592B3F9BA1EBA1D9F`;
- RC3 archived as
  `H:\luma\disabled\CthulhuFrameworkHistory\LumaHome010RC3-OVERBROAD-DEFER.firm`;
- root firmware unchanged;
- RC4 pushed to `lumahome/home-menu-framework`.

Test A-Z, Z-A, and A-Z in the same session before opening Notifications. The
unaffected top-level icons must move live each time and no wrong icon may take
another title's place. The stale folder-child icons may remain until HOME reload;
after Notifications, verify folder location/contents and repeat one live sort.

### 2026-08-12 — RC4 third-sort crash was Rosalina stack overflow; RC5 installed

RC4 completed the first two live refresh requests, but crashed on the third sort.
The RC4 report showed `inline_changed=164`, `indirect_changed=172`, two refresh
requests acknowledged, and no native callback failure. The 252-byte dump was
preserved locally as `runtime/.analysis/crash-rc4-partial-live-sort.dmp` before
removal from the SD card.

The dump identifies process `rosalina`, not HOME. Its saved PC was
`0x1403CF98`, inside Rosalina's BSS/non-executable region (the ELF executable
segment ends at `0x1402A5B8`). This is a corrupted return address. The runtime
thread has only an 8 KB stack, while RC4 added large automatic arrays: two
`CTH_PROCESSED_ENTRIES` record lists plus multiple 420-entry position/order
arrays. Repeated scans overflowed the thread stack and corrupted control flow.

RC5 commit `99c070a` preserves the RC4 partial live-sort algorithm but moves all
large desired/top-level/folder and inline/indirect work arrays into static BSS.
The scan now keeps only small counters and matched samples on the runtime stack.
Because the background sort is serialized, these static work buffers have no
concurrent consumer. No hook address, map permutation rule, refresh callback, or
persistent transaction behavior changed.

Build/deployment:

- visible title `LUMAHOME 0.1 RC5`;
- active payload `H:\luma\payloads\LumaHome010RC5.firm`;
- size 345600 bytes;
- SHA-256
  `237B2D534D94240DBFBD3EF7ACFC161305A213D2D98744FA3D4B6146BCA7467C`;
- RC4 archived as
  `H:\luma\disabled\CthulhuFrameworkHistory\LumaHome010RC4-CRASH-PARTIAL-MAP.firm`;
- RC3 remains archived as a safe no-live-update rollback;
- SD crash directory cleared only after preserving the dump locally;
- protected root firmware unchanged;
- RC5 pushed to `lumahome/home-menu-framework`.

Repeat A-Z, Z-A, A-Z, then Z-A (four sorts) in one session. This specifically
crosses RC4's third-sort failure point. Verify live movement/no wrong icons after
each, then Notifications reload and folder integrity. Stop immediately on any
crash and preserve the new dump.

### 2026-08-12 — RC5 stable; folder redraw absent; leading-folder collision fixed in RC6

RC5 crossed the prior third-sort crash point and produced no dump. Five native
refresh requests completed. The final A-Z report had `inline_changed=0` and
`indirect_changed=0` because the recognized live subsets were already in their
requested order. The user observed that folder contents did not redraw live and
one top-level icon remained out of order.

The persistent transaction proves folder contents themselves were sorted
correctly: Adventure Bar Story, VirtuaNES, Yoshi's New Island, Zero Escape, and
ZooVetPractice 3D had folder positions 0-4. Their visible order remained stale
because the live folder model is still not refreshed. This remains a known live
model limitation, not a persistence error.

The single displaced top-level title exposed an independent placement bug. With
one folder set to Before Titles, `firstTitlePosition` was 15 and the code assigned
both the folder and `3D Altered Beast` to position 15. The live map consequently
contained the folder special record at the boundary and omitted the title record
from the inline map. RC6 changes leading folder targets from
`firstTitlePosition + i` to
`firstTitlePosition - folderCount + i`. For the observed layout the folder now
uses empty position 14 and the first title remains at 15. Multiple leading
folders occupy the contiguous slots immediately preceding the first title.

Build/deployment:

- LumaHome commit `77293ca`;
- visible title `LUMAHOME 0.1 RC6`;
- active payload `H:\luma\payloads\LumaHome010RC6.firm`;
- size 345600 bytes;
- SHA-256
  `F00CEDD070094D1B57269BB538E5FC0334127C6F74236710AD51AA76789849C0`;
- RC5 archived as
  `LumaHome010RC5-STABLE-FOLDER-COLLISION.firm`;
- protected root firmware unchanged;
- RC6 pushed to `lumahome/home-menu-framework`.

Test Before+A-Z once, then Notifications reload. Verify folder at position 14,
`3D Altered Beast` at 15, and all following titles in order. Open the folder and
verify its persisted five-title order after reload. Then test After+Z-A once and
reload again. Live folder movement/contents are not yet expected before reload.

### 2026-08-12 — RC6 captured After+Z-A correctly placing folder at end

The user reported that the folder moved all the way to the back and reinserted
the card. The surviving (latest) transaction is unambiguously algorithm 2
(Z-A), folder placement After, with top-level title `Zelda: Seasons` ending at
position 201 and folder 0 assigned position 202. Therefore the observed back
placement is correct for the captured After operation, not evidence that RC6's
Before formula failed. The prior Before+A-Z transaction was overwritten by the
later log and cannot be audited from this card insertion.

RC6 remained stable: persistent result zero, two native refresh requests
acknowledged, and no crash dump. No code or payload change was made. To validate
the collision fix, perform only Before+A-Z, reload HOME through Notifications,
and reinsert before performing After; the transaction should show folder new=14
and first title new=15.

### 2026-08-12 — Isolated RC6 Before test proves reserved-gap issue; RC7 inserts

The isolated Before+A-Z transaction showed folder new=13 and first title
(`3D Altered Beast`) new=14. The targeted Launcher writer wrote and verified the
folder value successfully, and the SD title layout read back with zero byte or
position mismatches. Nevertheless HOME left the folder visually unmoved after
Notifications and showed additional gaps. This proves coordinates below the
first usable title position are not valid folder insertion targets on this HOME
layout; writing into the reserved leading area is accepted by the file format but
not interpreted as a visible folder slot.

RC7 commit `b3e0267` implements insertion instead of subtraction. For Before:

- the folder begins at the existing first-title position;
- every top-level title position is shifted forward by the number of folders;
- relative gaps between titles are preserved;
- bounds are checked and return `-105` rather than overflowing the 360-slot
  layout;
- the shifted positions are written into the normal persistent title records
  before grid rebuild and transaction commit.

For the captured layout this requests folder 14, first title 15, and shifts the
remaining top-level coordinates by one. After remains last-title+1 and is
unchanged.

Build/deployment:

- visible title `LUMAHOME 0.1 RC7`;
- active payload `H:\luma\payloads\LumaHome010RC7.firm`;
- size 345600 bytes;
- SHA-256
  `1B35ED5DBEB13807BD9607C133BA0301C345B28728BA839BE362C8A577C9226B`;
- RC6 archived as `LumaHome010RC6-RESERVED-GAP.firm`;
- root firmware unchanged;
- RC7 pushed to `lumahome/home-menu-framework`.

Test only Before+A-Z, then Notifications reload and reinsert. Expected log:
folder new=14, first title new=15. Visually the folder should precede the first
title without adding a leading reserved-area gap. Do not run After until this
isolated insertion is confirmed.

### 2026-08-12 — RC7 transaction valid but live Launcher stayed stale; RC8 syncs it

RC7 requested exactly the intended insertion: folder new=14 and `3D Altered
Beast` new=15, with all 168 top-level titles shifted forward by one. The
targeted Launcher file writer reported one planned/write/verified mutation and
result zero. However, after Notifications the folder remained visually at its
old location and extra gaps appeared. The persistence audit showed HOME had
rewritten all 168 title positions back (`position_mismatches=168`).

Direct snapshot parsing resolved the reconciliation source. The committed SD
snapshot had `3D Altered Beast` at 15, while the post-reload current snapshot had
it back at 14. The confirmed live Launcher resident object at `0x346DED88` still
reported folder 0 at old position 181. The existing code mapped and read this
object but never copied planned folder positions into it. Notifications did not
reconstruct that object in this lifecycle, so HOME reconciled the shifted titles
against the stale live folder and undid the insertion.

RC8 commit `ab956c4` adds a narrowly bounded live Launcher synchronization after
the persistent commits succeed and while HOME is still suspended. For each
validated planned folder it writes exactly one signed 16-bit position at
`launcherAddress + CTH_FOLDER_POSITION_OFFSET - 8 + folderId*2`, immediately
reads it back, and aborts with `-106` if verification fails. It flushes the local
and HOME process cache for the already-mapped Launcher object and logs address,
write count, and result under `[LIVE_FOLDER_WRITE]`. No folder names, numbers,
or other Launcher fields are modified.

Build/deployment:

- visible title `LUMAHOME 0.1 RC8`;
- active payload `H:\luma\payloads\LumaHome010RC8.firm`;
- size 345600 bytes;
- SHA-256
  `1B663AAE086BF7E3C301BA50848F4F97D4C1E366EEC63A978952B392AC1730EA`;
- RC7 archived as `LumaHome010RC7-LIVE-LAUNCHER-STALE.firm`;
- root firmware unchanged;
- RC8 pushed to `lumahome/home-menu-framework`.

Test only Before+A-Z. First observe whether folder/title movement occurs live;
then Notifications reload, verify folder before first title with no extra gaps,
and reinsert. Inspect `[LIVE_FOLDER_WRITE]`, folder new=14, first title new=15,
and the post-reload audit before testing After.
## 2026-08-14 — RC8 card inspection after folder-placement test

- The SD card mounted as `H:` and the active runtime identified itself as LumaHome `0.1.0-rc8` / runtime `1.8.4`.
- The completed sort transaction was found at the compatibility path `/3ds/Cthulhu/sort-transaction.txt` (not `/3ds/LumaHome/`).
- The transaction completed successfully (`result=00000000`, journal stage `shutdown-sort-committed`) with no crash.
- RC8 discovered Launcher at `346bf208` and its new bounded live folder-position write succeeded and read back (`[LIVE_FOLDER_WRITE] writes=1 result=00000000`). This validates the new direct live Launcher write mechanism.
- The actual completed transaction planned folder 0 at position 204 (`old=-1 new=204`). That is the **After titles** target, not the Before target. The title range in that transaction began at 16 and ended at 203.
- After reboot, `/3ds/LumaHome/runtime.txt` reports the overlay defaults (`direction=A-Z`, `folder_placement=before`, idle). These are freshly initialized per HOME Menu process and do not prove which option was active when the prior transaction ran.
- No firmware/code change was made during this inspection. Next test must explicitly apply one sort while the panel visibly reads `BEFORE TITLES`, then return the card without performing a second sort. Expected transaction target is the first usable coordinate (approximately 15), not 204. If it still records 204, instrument the apply-time boolean directly in the transaction and command channel before changing placement logic.

## 2026-08-14 — RC8 Before test diagnosis and RC9 authoritative Launcher source

RC8 Before+A-Z reached the sorter correctly (`folder_placement=before`, result 0, 173 mutations). It planned folder 0 at position 16 and shifted all top-level titles forward by one. The user observed the resulting title gaps and no live folder movement.

The logs isolate the cause: before planning, the resident Launcher object reported the existing folder position as `-1`, while the file-backed Launcher snapshot reports the same folder number 1 at position 204. RC8 successfully wrote 16 into the resident field and read it back, but HOME's live icon-grid record was not moved. Thus the title shift was real while the folder icon remained elsewhere.

RC9 changes:

- visible release `0.1.0-rc9`, runtime `1.8.5` / `active-home-controller-v185`;
- attempts a read-only authoritative `Launcher.dat` snapshot while HOME is locked, validates it, and uses it for folder planning when available;
- logs `[LAUNCHER_SOURCE]` with file result, validity, chosen source, and folder count;
- records the real old folder coordinate instead of forcing `old=-1`;
- refuses a Before insertion with result `-107` if a source folder coordinate cannot be validated, preventing another false-success title shift that creates gaps;
- live-map report bumped to scan 2.0.3 at `/3ds/LumaHome/live-map-0.1.0-rc9.txt`.

Build/deployment:

- active payload: `H:\luma\payloads\LumaHome010RC9.firm`;
- size: 346112 bytes;
- SHA-256: `29A965F340ACCB4D257EB9E3E545DA33F1FF5CE4E67C41F4EF76CBABE2FB8BB0`;
- RC8 archived as `LumaHome010RC8-LIVE-FOLDER-NO-GRID.firm`;
- protected root `H:\boot.firm` unchanged at SHA-256 `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Next test: boot RC9, confirm the visible RC9 label, select Before+A-Z, apply once, observe whether it succeeds or explicitly fails, then reinsert. Inspect `[LAUNCHER_SOURCE]`, folder old/new coordinates, and RC9 live-map. The next live-grid step is to identify/move the folder icon record using the authoritative old coordinate; do not reintroduce blind title insertion.

## 2026-08-14 — RC9 partial-live result and RC10 no-gap/idempotent planner

User-visible RC9 result: `Live sort partial, folder model stale, reopen HOME`; folder did not move live and gaps remained.

RC9 evidence:

- apply succeeded with Before+A-Z and 173 title mutations;
- authoritative file read was locked (`file_result=c92044e7`), so validated resident Launcher was used;
- resident folder 0 was now valid at old position 16 and RC9 targeted new position 17;
- the first title was old 17/new 18 and every later top-level title was likewise shifted one position;
- live-map found all 173 title records but explicitly reported five stale folder-member records, `live_update_partial=1`; inline changed 19 and indirect changed 148;
- shutdown folder commit failed with `ffffff9f`, while SD title persistence itself audited exact (`position_mismatches=0`).

This revealed that the RC7-RC9 insertion algorithm was not idempotent. Even when a folder was already before the first title, every Before application added another offset and manufactured another gap.

RC10 replaces offset insertion with an occupied-slot union/repartition:

- gather the existing coordinates of every top-level title and folder;
- sort that combined coordinate set without creating or deleting positions;
- Before assigns sorted folders to the earliest occupied coordinates and sorted titles to the remainder;
- After assigns titles to the earliest coordinates and folders to the latest;
- duplicate/invalid occupied coordinates abort with explicit `-108`/`-109` instead of writing;
- repeated Before or After application is idempotent and cannot walk the layout forward;
- visible release `0.1.0-rc10`, runtime `1.8.6`, scan `2.0.4`.

Deployment:

- active `H:\luma\payloads\LumaHome010RC10.firm`, 346112 bytes;
- SHA-256 `F59C3C3DD37C2EC2165F6E7670347F1485DB8C150B86841A8E92FBB0B0E4AEEC`;
- RC9 archived as `LumaHome010RC9-STALE-FOLDER-MODEL.firm`;
- root firmware unchanged.

Next test RC10 Before+A-Z once. It should not add gaps; with current folder old=16 and first title old=17, the folder should remain 16 and the first title 17. The live-model stale warning may remain until the folder icon record is incorporated into the inline/indirect model permutation. After confirming no drift, implement that record move using the same union ordering rather than offset insertion.

## 2026-08-14 — RC10 screenshots and RC11 automatic folder-record probe

The user supplied two 3DS screenshots after RC10. They showed one densely populated page and another page containing only a few icons with many empty coordinates. The folder was not at the requested live position. After inspection, exactly `HNI_0042.JPG` and `HNI_0043.JPG` were deleted from the SD card at the user's request.

RC10 transaction evidence:

- folder 0 resident old position 180, planned Before target 18;
- first title old 18, planned new 20 because the occupied-slot union preserves existing holes instead of collapsing them;
- live-map still reported `live_update_partial=1`, four stale folder-member records, inline changed 157 and indirect changed 147;
- shutdown folder commit again returned `ffffff9f`.

Interpretation: RC10 stopped manufacturing new coordinates, but intentionally preserved gaps already left by earlier iterations. Gap collapse remains a separate optional feature/repair. The folder icon itself is a non-title model record and is not part of the current 173-title permutation.

RC11 is a safe diagnostic iteration, not a speculative folder-model write. It records the inline and indirect record IDs found at the authoritative old folder coordinate, occurrence counts, whether each candidate is a top-level title or folder member, and requested placement. Output is in `/3ds/LumaHome/live-map-0.1.0-rc11.txt` as `folder_probe_inline` and `folder_probe_indirect`. This is intended to determine whether the same unique non-title record represents the folder in both maps before mutating it.

Deployment: visible `0.1.0-rc11`, runtime `1.8.7`; active payload `H:\luma\payloads\LumaHome010RC11.firm`; SHA-256 `37292581CBE6E44080EDFFE4439FB6D5902EEA6F3F70EA80D96ACF64C71A7348`; RC10 archived as `LumaHome010RC10-IDEMPOTENT-NO-FOLDER-MODEL.firm`; root firmware unchanged.

Next test: run Before+A-Z once under RC11 and reinsert. The display may still say partial/stale. Read both probe lines; only implement the live folder permutation if the candidate is a unique non-title, non-folder-member record with consistent identity across maps. Do not ask the user to transcribe it.

## 2026-08-14 — RC11 collision abort and RC12 read-only model enumeration

RC11 failed safely with `ffffff94` (`-108`) before any live scan or write. The resident Launcher claimed folder 0 was at coordinate 181, but Universal-Updater also occupied coordinate 181. This is direct proof that the resident Launcher folder coordinate is stale. The duplicate-coordinate guard prevented corruption; no crash occurred and no RC11 live-map was produced.

RC12 treats only this `-108` collision as a diagnostic continuation. It runs the live model scanner in probe-only mode while HOME remains locked, suppressing both inline and indirect mutations. In addition to the folder-coordinate probes, it logs up to 48 unique non-title record IDs, first indices, and occurrence counts for each map as `non_title_inline` and `non_title_indirect`. This should identify the folder icon record without trusting the colliding Launcher coordinate.

Deployment: visible `0.1.0-rc12`, runtime `1.8.8`; active `H:\luma\payloads\LumaHome010RC12.firm`; SHA-256 `B558B78577378DE9A5792C4DEC4088304BA2B737C6CC9739D3025140EB63D81C`; RC11 archived as `LumaHome010RC11-COLLISION-ABORT.firm`; root firmware unchanged.

Next test: apply Before+A-Z once. An on-screen failure `ffffff94` is expected, but `/3ds/LumaHome/live-map-0.1.0-rc12.txt` must now exist. Reinsert and compare the two folder probes and non-title lists. RC12 should not move any icons or add gaps.

## 2026-08-14 — RC12 identifies live folder records; RC13 guarded move

RC12 completed successfully on the next layout state. It identified the folder representation at old model index 181 and requested Before target 13:

- inline map: record 190, exactly one occurrence, neither title nor folder-member record;
- indirect map: record 360, exactly one occurrence, neither title nor folder-member record;
- index 13 was empty in both relevant maps;
- no crash; persistent sort committed.

The differing record IDs are expected because inline and indirect maps use separate representations. RC13 performs an independently validated swap in each map: old/new indices must be in range, old and new must differ, the source record must be nonnegative, unique, and absent from all title/folder-member record sets. The target entry is moved back to the old index rather than discarded, preserving map membership. Both swaps and displaced IDs are logged as `folder_move_inline` and `folder_move_indirect`; cache flushes cover folder-only changes.

Deployment: visible `0.1.0-rc13`, runtime `1.8.9`; active `H:\luma\payloads\LumaHome010RC13.firm`; SHA-256 `E68003F433B05FEF491512BC9C82912165D8572BFCE9E9A09508A8CF47F2BEEC`; RC12 archived as `LumaHome010RC12-FOLDER-RECORD-PROBE.firm`; root firmware unchanged.

Next test: Before+A-Z once. Observe whether the folder moves live to the leading slot and whether HOME remains responsive. Existing gaps are not expected to collapse in this iteration. Reinsert for the two move logs and crash check.

## 2026-08-15 — RC13 screenshot/result and RC14 stale-inline recovery

The user supplied `HNI_0044.JPG`; it showed the folder still at its old on-screen location while the first visible user slot contained the warning icon. The screenshot was inspected and then deleted from the SD card as requested.

RC13 evidence:

- sort and shutdown persistence succeeded with no crash;
- Launcher now reports folder old=13/new=13;
- indirect map already contains the validated folder record 360 at index 13, so no indirect move was needed;
- inline map at index 13 contains title record 15, while the previously validated unique inline folder record 190 remains at index 181;
- because Launcher said old==new, RC13 skipped the inline move even though the inline representation was still stale.

RC14 adds a narrowly guarded recovery for this already-proven split state. If Launcher says old==new but the inline target is a title, it searches for the RC12-validated inline folder record 190. Recovery proceeds only if record 190 occurs exactly once. Its actual source index is used for the membership-preserving swap, the displaced target record is retained at the old source, and the normal title permutation then reorders that displaced title. The report now includes source index and `recovery_hint=1`.

Deployment: visible `0.1.0-rc14`, runtime `1.9.0`; active `H:\luma\payloads\LumaHome010RC14.firm`; SHA-256 `125A5C9DADEEE5E2378868730B1D89C0884A90524500414ABF34B0449921C585`; RC13 archived as `LumaHome010RC13-INDIRECT-MOVED-INLINE-STALE.firm`; root firmware unchanged.

Next test: Before+A-Z once. Expected RC14 log is inline old=13, source=181, new=13, record=190, recovery_hint=1, moved=1; indirect should remain record360 at13, moved=0. Visually the folder should move live to the leading slot. Existing empty slots are not collapsed by this test.

## 2026-08-15 — RC14 live Before folder placement visually confirmed

The user confirmed the folder moved live to the front under RC14. After was not tested because the device was charging.

The successful run occurred after a HOME process/model rebuild (pid 40, raw `346f9d20`, processed `346fcad0`, Launcher `346ded88`). In this model state Launcher reported folder old=18/new=14. The inline coordinate probe at 18 found a title, so no inline folder move was attempted. The indirect layer positively identified unique non-title folder record 360 at 18 and moved it to 14, preserving displaced title record 15. The user-visible folder movement therefore validates the indirect-layer guarded swap as the operative live placement mechanism. No crash dump was present.

Caveat: the shutdown folder persistence stage returned `ffffff9f`. The subsequent persistence audit reported 172 title-position mismatches between current extdata and the expected snapshot, although the resident raw/processed live buffers matched. Treat live Before placement as proven, but do not yet claim reboot persistence for this run.

No firmware change was made after this result. RC14 remains the active baseline. Next tests, after charging: (1) verify current folder position after reboot/normal HOME load; (2) test After+A-Z live once; (3) reinsert and inspect both model-move and shutdown persistence logs. Existing gaps remain a separate optional collapse/repair feature.

## 2026-08-15 — Folder disappeared after reboot; RC15 targeted persistence repair

The user reported that the folder disappeared after reboot. Current SD title extdata remains internally exact (`position_mismatches=0`), including the five titles assigned to folder 0, so folder contents were not deleted. The missing UI folder is a Launcher position failure.

RC14's targeted shutdown writer failed with `ffffff9f` (`-97`). Its identity guard required both folder number and the old position to match. HOME can update/invalidate that position before Rosalina handles the shutdown notification, making the old-position comparison inherently racy. The live folder move succeeded, but the two-byte persistent Launcher position did not commit.

RC15 changes:

- targeted Launcher writes identify a folder by its stable folder number only; they no longer reject a valid folder because its current position differs from the earlier live position;
- a nonzero folder number with an invalid position is treated as an unplaced/recoverable folder rather than error `-44`;
- missing-folder Before repair uses only the free slot immediately preceding the first title; After uses only free space immediately following the last title;
- repair does not shift or rewrite title coordinates and aborts with `-110`/`-111` if no safe free target exists;
- only the two-byte folder position is written at shutdown, with existing number verification and readback.

Deployment: visible `0.1.0-rc15`, runtime `1.9.1`; active `H:\luma\payloads\LumaHome010RC15.firm`; SHA-256 `C577BDD994F9951ECC81D6DAA46CE8B85249E8BBC6B8EC400849AF73DF59562B`; RC14 archived as `LumaHome010RC14-LIVE-FOLDER-PERSIST-FAILED.firm`; root firmware unchanged.

Recovery test: boot RC15, apply Before+A-Z once. If the folder is currently absent, live display may still require HOME reopen/reboot because no folder model record may exist to move. Power off normally so the shutdown handler can write and verify the two-byte position, then reboot and confirm the folder reappears before testing After.

## 2026-08-15 — RC15 persistence recovery confirmed; label display remains

User result: the folder appeared on initial RC15 boot, survived sort → normal power-off → power-on, and did not vanish. The user was unsure of exact placement and reported the displayed folder name missing.

Logs confirm the persistence repair succeeded completely: folder 0 number 1 remained at old=15/new=15; targeted Launcher commit opened archive/file, wrote once, verified once, committed, and returned 0; shutdown journal stage is `shutdown-sort-committed`; no crash exists. First top-level title position is 18, so folder position 15 is correctly before all titles.

The pre-sort Launcher snapshot contains folder 0 name bytes `67 00 65 00 73 00 73 00 ...`, UTF-16LE `gess`, at documented offset `0x1560`. Number is 1 and position is 15. RC15 changed only the two-byte position and did not erase the name. Therefore the missing visible label is a HOME live/model label refresh issue, not persistent name loss.

RC15 remains unchanged as the proven reboot-persistent Before baseline. Next work: test After separately, then diagnose/rebuild the live folder label representation. Do not restore Launcher.dat or rewrite name bytes because the authoritative name is intact. Existing gaps remain separate.

## 2026-08-15 — RC15 After placement and name preservation fully confirmed

The user clarified that the full After test succeeded: the folder appeared after the individual titles, survived reboot, and retained its name. Card logs corroborate the visual result.

RC15 After+A-Z transaction: result 0; folder 0 number 1 moved old=15 to new=205; first title moved into position 15 and the remaining titles reused the existing combined occupied coordinates. The indirect live model uniquely identified folder record 360 at index 15 and moved it to 205 while preserving displaced record 182. The inline map did not perform a speculative move because its old coordinate contained a title. The visible result was nevertheless correct.

Targeted Launcher shutdown persistence opened successfully, wrote one two-byte position, verified one write, committed the archive, and returned 0. Journal stage is `shutdown-sort-committed`. Folder name remained intact, so no label repair is presently required.

RC15 is now the proven stable baseline for both Before and After folder placement, live display, normal power-off/reboot persistence, contained-title preservation, and folder-name preservation. Remaining planned feature work begins with optional gap collapse, followed by row/column traversal choices, top/bottom folder rows, special A-Z folder organization, and eventually live search/filtering.

## 2026-08-15 — RC16 optional Collapse Gaps implementation

RC16 adds a third overlay field, `COLLAPSE GAPS`, default OFF. Direction and folder placement remain independent. Navigation now cycles through Direction, Folder Placement, and Collapse Gaps; left/right toggles the selected value.

When OFF, RC15's proven occupied-coordinate behavior is retained. When ON:

- top-level titles and folders are assigned a contiguous range beginning at HOME's first user coordinate 13;
- Before assigns sorted folders first, then sorted titles;
- After assigns sorted titles first, then sorted folders;
- each folder's contained titles is compacted independently from position 0;
- capacity overflow aborts with `-112` rather than writing;
- no folder name, number, membership, title ID, or unrelated Launcher field is changed.

The collapse choice is passed explicitly into the background sorter and recorded in `runtime.txt` as `collapse_gaps=on/off`. Visible release `0.1.0-rc16`, runtime `1.9.2`, live scan `2.1.0`.

Deployment: active `H:\luma\payloads\LumaHome010RC16.firm`; SHA-256 `273AD099649E5B2CC514A834536289F75205F2C474666E754DA901AD5543549F`; RC15 preserved as `LumaHome010RC15-STABLE-BIDIRECTIONAL.firm`; root firmware unchanged.

Test order: boot and confirm RC16; set Before+A-Z and Collapse Gaps ON; apply once; verify folder is first, all top-level gaps are removed, folder contents are A-Z with no gaps, and HOME remains responsive; power off normally, reboot, verify persistence, then reinsert. Do not test After until this first compacting transaction is audited.

## 2026-08-15 — RC16 gap collapse persistence succeeds; special icons remain

User result: sort succeeded; gaps did not close live; after reboot most gaps closed, but the user's described "unwrapped games" did not move.

Logs prove the serialized compaction itself is exact: folder 0 moved 205→13; all 168 top-level cataloged titles were assigned contiguous positions 14–181; all five folder members were assigned 0–4. Targeted Launcher commit wrote/verified once and returned 0. Shutdown committed. Reboot audit reports byte mismatches 0 and position mismatches 0. No crash.

The live model explains the partial visual update: all 173 desired title records were found in the full model, but the inline map contained only 162 of 168 top-level records and still exposed five folder-member records. It changed 148 inline entries. The indirect map contained all 173 records, moved folder record 360 from 205 to 13, and needed no additional title permutation. Therefore the current live updater cannot close gaps for six top-level records absent from the active inline page/model.

Any icons still unmoved after reboot are outside the 173 ordinary serialized title records whose positions audit exact. They may be HOME's wrapped/present or another special icon state; do not blindly treat the remaining non-title records as titles. Next diagnostic should log the six top-level records missing from inline plus the remaining special model records and correlate them with a screenshot/precise wrapped-vs-unwrapped description before mutating them.

RC16 remains installed. Collapse OFF remains the RC15-compatible behavior; Collapse ON is proven persistent for ordinary titles and folder contents, but live redraw and special-icon coverage are partial.

## 2026-08-15 — Remaining icons identified as gift-wrapped packages

The user clarified that the icons which did not move were games still displayed as gift-wrapped packages, and then unwrapped them. This confirms they were temporary HOME presentation/package records rather than ordinary cataloged title-position records. RC16 correctly did not mutate those unknown non-title model records.

Do not add special wrapped-package sorting based on the current non-title IDs: the state is temporary, record identity is not proven stable, and the packages become normal sortable titles after unwrapping. Next test should rerun RC16 Before+A-Z with Collapse Gaps ON now that all packages are unwrapped. Expected result: the newly normal titles join the catalog and compacted range; compare request/title count and verify live/reboot placement. If gaps remain after that, log only the specific missing ordinary records.

## 2026-08-15 — RC17 selectable row/column traversal

The user asked to proceed to the next version after unwrapping the remaining gift packages. RC17 implements the next planned feature: traversal order is now independent of alphabetic direction, folder placement, and gap collapse.

The overlay has a fourth `TRAVERSAL` field:

- `COLUMN: DOWN/RIGHT` is the default and preserves RC16's existing linear-coordinate behavior;
- `ROW: RIGHT/DOWN` sorts visually left-to-right across a row before proceeding down.

The implementation follows 3dbrew's Launcher.dat description: offset `0xB51` stores HOME's row count minus one, positions are linear, and horizontal columns are derived by dividing by the number of rows. RC17 reads and validates the active HOME row count (1–6) instead of assuming a grid height. Each folder uses its own validated row count from `0x1434 + folderId`. Invalid row metadata aborts before commit with `-113` (HOME) or `-114` (folder).

With Collapse Gaps OFF, RC17 orders the already-occupied coordinates by the selected visual traversal, preserving the exact coordinate set and holes. With Collapse Gaps ON, it generates a compact coordinate rectangle in the selected traversal. Folder Before/After continues to determine which sorted group receives the leading or trailing ranks, and folder contents use the same selected traversal as top-level titles. Column mode deliberately takes the pre-RC17 code path.

Logging now records `traversal=row-major/column-major` in `/3ds/LumaHome/runtime.txt` and a `[TRAVERSAL]` section with the decoded HOME row count in the transaction details. Visible release is `0.1.0-rc17`, runtime `1.9.3`, live scan `2.1.1`.

Build succeeded with the existing `cthulhu-luma-toolchain:1` Docker image. Deployment: active `H:\luma\payloads\LumaHome010RC17.firm`; SHA-256 `0C13716D4FDFA5F2CEBDF0D4BA22A296BA9DE6CFB3EC06F20A0DFA8FB77AB751`; RC16 archived as `LumaHome010RC16-COLLAPSE-GAPS.firm`; root `H:\boot.firm` remains unchanged with SHA-256 `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Focused RC17 test: boot the RC17 payload and confirm the panel title. Set A-Z, Before, Collapse Gaps ON, and `ROW: RIGHT/DOWN`; apply once. Verify names advance alphabetically left-to-right on the first visual row and then continue on the next row, the folder remains before titles, folder contents use the same traversal, and HOME remains responsive. Then normal power-off/reboot and verify persistence. Reinsert the card after this one transaction so the row counts, desired positions, live-map coverage, and shutdown commit can be audited before trying column mode or Z-A.

## 2026-08-15 — RC17 row origin bug confirmed; RC18 absolute-grid correction

User result and screenshots `HNI_0046.JPG` through `HNI_0048.JPG`: row traversal sorted consistently but began the A titles on the third/fourth visual row and later wrapped back to the top. The three screenshots were inspected and removed from the SD card afterward per the standing diagnostic-image request.

Logs confirmed `mode=row-major home_rows=5`, one folder, 168 top-level titles, exact serialized persistence (`byte_mismatches=0`, `position_mismatches=0`), and successful shutdown commit. The failure was therefore purely coordinate interpretation: RC17 subtracted movable origin 13 before calculating row/column. On a five-row grid, coordinate 13 is physical column 2, row 3—not a visual row boundary.

RC18 calculates traversal keys against HOME's absolute grid origin 0. For compact top-level assignment it enumerates only the safe contiguous movable range `[13, 13 + count)` in absolute visual row order. This prevents writes into fixed coordinates 0–12 while allowing the first alphabetical title to occupy the earliest top-row coordinate actually available. The two partial cells at positions 13 and 14 naturally occur later in absolute row traversal rather than incorrectly defining its beginning. Folder-contained positions already begin at zero and retain the same behavior. Column traversal remains unchanged.

Visible release is `0.1.0-rc18`, runtime `1.9.4`, live scan `2.1.2`. Build succeeded. Deployment: `H:\luma\payloads\LumaHome010RC18.firm`; SHA-256 `162E925023C9D28BEBAC19C68A0172D271232D2420C62F7334C10C346137977A`. RC17 is archived as `LumaHome010RC17-MIDCOLUMN-ROW-ORIGIN.firm`. Root `H:\boot.firm` remains unchanged with SHA-256 `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Next test: boot RC18, confirm the visible version, and repeat A-Z + Before + Collapse ON + Row Right/Down once. Verify A titles begin on the first available top-row cell and progress horizontally without later wrapping back to earlier rows. Also inspect the last two cells of the partial first movable column: they should contain late-sort titles because those cells occur on lower rows in absolute row-major order. Verify folder placement and contained-title ordering, normal power-off/reboot persistence, then reinsert the card.

## 2026-08-15 — RC18 visual traversal passes, but Collapse was OFF

User report: the main row traversal looked correct after the origin fix, mGBA appeared gift-wrapped again, and one item (screenshot shows TM++) remained far from the other icons. Screenshot `HNI_0049.JPG` was inspected and removed afterward.

The runtime log proves this test used RC18 row-major A-Z Before but `collapse_gaps=off`. Therefore the isolated coordinate is expected to remain: OFF intentionally preserves the complete preexisting occupied-coordinate set and all holes while only changing which title occupies each coordinate. This was not a failure of live record discovery. The live report found all 173 desired records, had `desired_missing=0`, changed all 168 top-level inline records and all 168 indirect records, and reported `live_update_partial=0` / `live_update_deferred=0`. Folder movement was also applied in the indirect model. Do not change the ID map based on the distant icon.

The mGBA gift state is not explained by the position-only mutation: RC18 changes only the two-byte title position fields and the folder position. It does not write title flags or package state. Treat it as a separate reproducibility question. Next test must explicitly isolate both observations: unwrap mGBA first; open RC18 and set A-Z, Before, **Collapse Gaps ON**, Row Right/Down; apply once; note whether mGBA becomes wrapped immediately and whether TM++ joins the compact arrangement; then normal power-off/reboot and check both again. This will distinguish a sort-time transition from HOME's reboot/install-state handling while finally testing compact row traversal.

## 2026-08-15 — RC19 remembers overlay choices and logs applied choices

The user requested that the sort option be remembered after the RC18 runtime log misleadingly showed reset defaults following a HOME/reboot cycle. RC19 persists all four independent overlay choices: direction, folder placement, Collapse Gaps, and traversal.

Settings are stored in `/3ds/LumaHome/settings.bin` as a fixed 12-byte structure with magic `LMHS`, version 1, bit flags, and a validation check word. The file is rewritten and flushed whenever left/right changes a field and again when A applies. Each new HOME runtime loads it before the first overlay render. A missing, short, wrong-version, or invalid-check file is ignored safely and defaults remain A-Z, Before, Collapse OFF, Column.

RC19 also writes `/3ds/LumaHome/last-sort-options.txt` immediately before calling the sorter. This durable text records the exact direction, folder placement, collapse value, and traversal passed to that transaction, so later runtime reattachment cannot obscure the applied choice.

Visible release `0.1.0-rc19`, runtime `1.9.5`, live scan `2.1.3`. Build succeeded. Deployment: `H:\luma\payloads\LumaHome010RC19.firm`; SHA-256 `1B18159F6EF4076E343BF79F7590E41EFB2E2C8756C1D287AB63E8D4FCA63A98`. RC18 archived as `LumaHome010RC18-ABSOLUTE-ROW-GRID.firm`; root boot firmware unchanged.

Next test: boot RC19, set A-Z + Before + Collapse ON + Row Right/Down, close the panel, reopen it and confirm all four choices remain. Apply once. Reopen again or restart HOME and confirm the values still remain. Then inspect whether the distant TM++ icon compacts and whether mGBA wraps; reinsert the card so `settings.bin`, `last-sort-options.txt`, live-map, and persistence logs can be compared.

## 2026-08-15 — RC20 remembers only the last applied choices

The user clarified the persistence semantics: after reboot, the selected values must be the values used by the last Apply, not merely the last values edited in the panel.

RC20 removes settings writes from left/right field changes. Edits remain active in the current attached runtime, including closing and reopening the overlay in the same HOME process, but they do not replace the saved defaults. Pressing A writes and flushes all four selected choices immediately before the sort attempt and writes the matching durable `last-sort-options.txt`. A later HOME restart or full reboot loads that applied combination from `settings.bin`. This also means changing a field and rebooting without pressing A restores the preceding applied combination.

Visible release `0.1.0-rc20`, runtime `1.9.6`, live scan `2.1.4`. Build succeeded. The SD card was not mounted at installation time (H: absent), so the compiled firmware is currently only at `runtime/.build/Luma3DS/boot.firm`; RC20 still needs to be copied to the SD payload directory. Do not claim it is installed until its destination hash is verified.

Next test after installation: apply a recognizable combination such as Z-A + After + Collapse ON + Row. Change one field without applying, then reboot. The overlay should restore exactly the applied combination, not the later unapplied edit. After that, return to A-Z + Before + Collapse ON + Row and apply for the pending compact-layout/mGBA test.

## 2026-08-15 — RC20 installed after SD remount

The card remounted as H:. RC20 was copied to `H:\luma\payloads\LumaHome010RC20.firm` and verified SHA-256 `EA2952D30F41ADD0F90F61B407A6E13CB8FA54594D96ED4C8EC1403711385D67` (349,696 bytes). RC19 was preserved as `LumaHome010RC19-REMEMBER-EDITED.firm`. Root `H:\boot.firm` remains unchanged with SHA-256 `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

## 2026-08-16 — First post-install boot was still RC19

After the user remounted the card, inspection showed `runtime.txt` release `0.1.0-rc19`, runtime `1.9.5`; no RC20 live-map or applied-options file exists. Therefore RC20 was present but was not selected in the chainloader, and its semantics remain untested.

The existing 12-byte `settings.bin` is valid RC19 data with flags `0x000e`: A-Z, Before, Collapse ON, Row. It was last written at 22:40:34 by RC19's edit-time persistence and cannot serve as proof of RC20's Apply-only behavior. RC19's persistence audit is exact (0 byte and position mismatches). Next action is simply to chainload `LumaHome010RC20.firm`, confirm the visible RC20 title, then conduct the applied-versus-unapplied reboot test documented above.

## 2026-08-16 — RC20 exposes excluded TWiLight/special-icon boundary

The user booted RC20 and applied Z-A + After + Collapse ON + Column. `last-sort-options.txt` and `runtime.txt` agree exactly, proving the applied settings persistence/logging works. The sorter returned 0 with 173 mutations. Screenshot `HNI_0050.JPG` showed TWiLight Menu++ isolated and the folder visually not after every application.

The transaction planned 168 top-level SD titles at positions 13–180 and folder 0 at 181. Both live layers found all desired records (`desired_missing=0`), changed all 168 top-level entries, moved the folder 15→181, and reported no partial/deferred update. Thus folder placement is exactly after the current sortable SD set, not after every visible HOME icon.

TWiLight Menu++ is not present in the 332-entry AM-generated sort request (no `000480...` entry was returned), while the live maps retain a unique non-title model record 205 at coordinate/index 14. This excluded HOME/DSi-style icon is the common cause: it is not compacted or alphabetically ranked, and its presence makes the otherwise-correct folder boundary look wrong. mGBA is separately confirmed as ordinary SD title `00040000001A1E00`, slot 8, and was sorted old22→new113 in this Z-A transaction.

Do not adjust folder rank or compact arithmetic to hide this. Next work is to identify record 205 safely and add supported DSiWare/special HOME application discovery to the combined top-level planner. First instrumentation should dump all words/identity fields for every unmatched live model record (especially 205), correlate them with current Launcher entries and visible coordinate, and retain the full result automatically. Only after stable title identity is proven should it enter persistent Launcher mutation and alphabetical naming. This also makes Before/After mean relative to all supported applications rather than SD-only titles.

## 2026-08-16 — RC21 unmatched movable-icon identity capture

The user established the requirement that TWiLight/DSiWare and similar application icons be treated the same as ordinary icons. The architecture is now explicitly: one visible movable-icon ordering, with icon type used only to select discovery and backing-store persistence. Fixed HOME system buttons remain excluded.

RC21 is an instrumentation release and intentionally retains RC20's proven mutation behavior. During every Apply, while HOME is atomically paused and the full icon model is mapped, the live-map report now expands every unique record not matched to the SD title grid. For each record it logs its current coordinate, occurrence count, 64-bit ID-shaped words 0/1, state words 2–7, and word 14. This covers record 205 and every other unmatched class in one run and avoids hard-coding a transient record index.

Visible release `0.1.0-rc21`, runtime `1.9.7`, scan `2.2.0`. Build succeeded. Deployment: `H:\luma\payloads\LumaHome010RC21.firm`; SHA-256 `F7B4CA0F6B819C6CAAEF23AE739D431D079725CB77D0BE2364A011758F1D276F`. RC20 archived as `LumaHome010RC20-LAST-APPLIED.firm`; root boot firmware unchanged.

Next test: boot RC21 and confirm the visible version. Apply the currently remembered options once; no special rearrangement setup is required. Confirm HOME remains responsive, then reinsert the card. Inspect `live-map-0.1.0-rc21.txt`, especially the expanded `non_title_inline` entry for record 205, to determine stable TWiLight identity and distinguish folders/fixed system buttons from other movable applications. The following implementation should merge proven movable records into catalog ranking and folder boundaries through their correct persistent store.

## 2026-08-16 — RC21 proves stale shutdown replay; RC22 preserves post-sort edits

User result: after RC21 sort, manually placing an icon into a folder and unwrapping/rebooting did not persist. The next wrapped title was 3D Classics Kid Icarus, and the user correctly inferred that a location—not a particular title—was repeatedly becoming wrapped. Screenshot `HNI_0051.JPG` was inspected and removed with its BIN sidecar.

RC21 identity capture succeeded. The excluded movable records are not anonymous: record 13 is DSi ID `0004800023232323`; record 14 is DSi ID `0004800453524C41`. Record 205 at coordinate 14 is a blank/special presentation record (`ffffffffffffffff`, `w14=1`), consistent with HOME's gift/package proxy. All ordinary desired records were present in the full model, but the inline map changed only 167/168 because the proxy occupied the coordinate.

The primary data-loss cause was found in the shutdown handler. Apply already commits and verifies the sorted extdata immediately, but `pending-sort-commit.bin` contains the whole sorted SaveData snapshot. At shutdown RC21 reread that stale snapshot and wrote the entire file again. Any user changes made after Apply—including unwrap/package state and folder membership—were overwritten even though the position-only audit later appeared exact.

RC22 replaces full-snapshot shutdown replay with an identity-based merge. It loads the pending desired snapshot, rereads HOME's current SaveData, and matches every title ID uniquely. If folder membership is unchanged, only each matching title's two-byte sorted position is copied into the current file; all presentation/install state and unrelated bytes remain current. If any title's folder membership differs, no planned positions are replayed at all because HOME may have shifted both affected groups; the complete current layout is preserved and committed. Ambiguous/missing title identity aborts with `-115`.

`/3ds/Cthulhu/shutdown-merge.txt` records matched count, membership-change count, position writes, merge mode, and result. Visible release `0.1.0-rc22`, runtime `1.9.8`, scan `2.2.1`. Deployment: `H:\luma\payloads\LumaHome010RC22.firm`; SHA-256 `6137C14E279187E51D0A7F19673ABF237EFEE1EE52318A1B556B6A94FED1E2DF`. RC21 archived as `LumaHome010RC21-UNMATCHED-IDENTITY.firm`; root firmware unchanged.

Focused RC22 regression test: boot RC22; unwrap the current package; Apply A-Z + Before + Collapse ON + Column once; after Apply, move one clearly identified title into the existing folder; normal power-off/reboot. Verify (1) the unwrapped/package state did not revert or migrate to the title now occupying that coordinate, and (2) the manually moved title remains in the folder. Then reinsert. Expected shutdown log: `membership_changes` at least 1, `position_writes=0`, `mode=preserve-current-layout`, result 0. Only after this preservation fix passes should DSi records 13/14 be added to combined sorting.

## 2026-08-16 — RC22 preservation confirmed; DSi inclusion is next

User confirmed both RC22 regression targets: the manually moved title remained in its folder after reboot, and icons/package state stayed where the user left them. `shutdown-merge.txt` exactly matches the intended guard: 173 identities matched, `membership_changes=1`, `position_writes=0`, `mode=preserve-current-layout`, result 0. The stale shutdown replay bug is fixed.

TWiLight Menu++ still did not move, which is expected because RC22 did not yet add DSi records to the desired set. RC21 proved two live movable DSi records: record 13 ID `0004800023232323` and record 14 ID `0004800453524C41`; the latter has the standard `00048004` DSiWare class prefix. The AM-generated catalog did not include either. Next implementation must enumerate these from the validated Launcher/live model, obtain names from their DSi banners or a safe alias fallback, merge them with SD titles before assigning coordinates, move their live records, and persist their positions via narrowly targeted Launcher title-position writes keyed by title ID. Folder Before/After boundaries and collapse counts must include them. Do not return to full Launcher or SaveData snapshot writes.

## 2026-08-16 — RC23 combined DSi application sorting

RC23 adds the two validated movable DSi applications to the same mutation/ranking pipeline as SD titles. Stable Launcher evidence from the current device: `0004800023232323` is Launcher slot 12 at old position 166; `0004800453524C41` is slot 13 at old position 205. The second is the TWiLight/SRLoader-class icon; aliases are currently `TWiLight Menu++ Game Booter` and `TWiLight Menu++` respectively until banner-name extraction replaces them.

The common mutation structure now records backing media. Both DSi IDs are injected into the in-memory sorted request, resolved uniquely against validated Launcher title slots, and participate in the same top-level position pool, A-Z/Z-A comparison, Collapse, row/column traversal, and folder Before/After boundary. SD positions still write only SaveData position fields. DSi positions write only Launcher offset `0xD9A + slot*2`.

The final desired live grid overlays the DSi title IDs at their newly planned positions, allowing the existing full-model identity matcher to move their actual records rather than treating them as non-title obstacles. Live Launcher writes use the resident offset equivalent and verify readback.

Folder plan format is version 3 and carries up to 16 targeted Launcher-title mutations. At normal shutdown, each DSi write first rereads and verifies the exact 64-bit title ID at the planned slot, writes only its two-byte position, and verifies readback. Errors `-116` through `-118` abort targeted persistence. Folder writes remain title-independent and RC22's safe SaveData merge remains unchanged. No full Launcher snapshot is written.

Visible release `0.1.0-rc23`, runtime `1.9.9`, scan `2.3.0`. Build succeeded. Deployment: `H:\luma\payloads\LumaHome010RC23.firm`; SHA-256 `41BC7E7F90B314B4EBC4C0170534722DC8073489AF745C29F31B211B7F1DE04E`. RC22 archived as `LumaHome010RC22-SAFE-SHUTDOWN-MERGE.firm`; root firmware unchanged.

Focused RC23 test: boot and confirm RC23. Use A-Z + Before + Collapse ON + Column and Apply once. Verify TWiLight Menu++ and the second DSi icon move live into the T section, the existing folder remains before every supported application, no icon wraps, and HOME remains responsive. Normal power-off/reboot; verify both DSi positions, folder membership, folder name, and icon presentation persist; then reinsert. If Apply fails or display is partial, stop and collect the result without attempting a second direction.

## 2026-08-16 — RC23 persisted DSi positions but blank names sorted last; RC24 alias fix

User result: immediately after Apply, icons did not fully compact (`HNI_0052.JPG`); after reboot they compacted, but TWiLight remained at the end/wrong location (`HNI_0053.JPG`). Both screenshots and sidecars were inspected and removed.

Facts from logs: RC23 applied A-Z + Before + Collapse ON + Column. Targeted Launcher commit verified one folder plus two DSi position writes. The transaction included DSi slots 12/13 but logged both names empty and assigned them positions 181/182. The full live model found 169 top-level records with no desired identity missing, but the inline map changed only 168 and reported `live_update_partial=1`; the indirect map changed 174. Thus persistent compaction worked after reboot, while complete live compaction remains blocked by the coordinate-14 gift/proxy representation.

The DSi naming bug was an implementation error: RC23 aliases were added only when an ID was absent from the request. The request already contained both DSi IDs as unnamed entries, so aliases were not applied and the comparator correctly placed unknown names last. RC24 now fills an alias whenever the matching existing entry is unnamed, as well as appending it when absent. Expected aliases: `TWiLight Menu++` and `TWiLight Menu++ Game Booter`.

Current assumptions, explicitly tracked:

1. Launcher title ID is stable identity for DSi persistence. Strongly supported by unique slots, live model IDs, and verified targeted readback.
2. `0004800453524C41` is the visible TWiLight/SRLoader icon and `0004800023232323` is its game-booter companion. Supported by DSi class/product history and observed last-page icons, but banner extraction is still needed to eliminate alias inference.
3. Position 13 is the first user-movable coordinate and 0–12 are fixed system entries. Repeated layout evidence supports this.
4. Launcher linear coordinates are column-major with the active row count. Supported by five-row screenshots and corrected RC18 behavior.
5. Record 205 is a gift/package presentation proxy occupying coordinate 14, not a sortable application identity. Supported by blank ID, `w14=1`, and title-dependent wrapping at that coordinate; exact backing flag is not yet known.
6. The inline map drives immediate visible layout, while the serialized raw/processed model plus Launcher drives reconstruction after reboot. Strongly supported by partial-live/full-reboot results.
7. A safe live collapse cannot simply overwrite record 205; its ownership/association must be understood or cleared through HOME's native package-state path. This is a safety assumption, not yet proven.
8. When manual folder membership changes after Apply, preserving HOME's whole current SD layout is safer than replaying planned positions. RC22 test and shutdown merge log prove this behavior preserves user changes.

Visible release `0.1.0-rc24`, runtime `1.10.0`, scan `2.3.1`. Deployment: `H:\luma\payloads\LumaHome010RC24.firm`; SHA-256 `D2E45DAAF272DA9991B515A6F48748826907A13FEAD3FA8B8F3DB9664A64ED9C`. RC23 archived as `LumaHome010RC23-DSI-BLANK-ALIASES.firm`; root firmware unchanged.

Next focused test: boot RC24, Apply A-Z + Before + Collapse ON + Column once, and check only whether TWiLight appears in the T section after reboot. Live gaps may remain and are not the target of RC24. Reinsert afterward; transaction lines for both DSi IDs must show nonempty names and T-range target positions before banner extraction/live-proxy work continues.

## 2026-08-17 — User corrected three unsupported architectural assumptions

The user challenged three assumptions from RC24. Treat the corrections below as binding design direction; do not extend the hard-coded approach.

1. Specific TWiLight/booter IDs should not be required. RC23/24 hard-coded two IDs because AM omitted/unnamed them and the live diagnostic exposed them. That was acceptable only as a narrow experiment, not a framework design. The correct implementation must generically enumerate visible movable records from validated HOME/Launcher structures, obtain stable identity and banner/display name through the appropriate title type, and dispatch persistence by backing store. No product-specific ID list should define sort eligibility.
2. Coordinates 0–12 are not intrinsically fixed. They were repeatedly occupied by HOME/system records in this device's snapshots, and the implementation mistakenly converted that observation into a protected-coordinate rule and compact origin 13. The user can manually move those icons, proving they are movable UI objects. The next model must distinguish movable versus fixed by object semantics/flags, not coordinate number, and build its available-coordinate set from the actual visible model.
3. Record 205 is not proven to be a coordinate-14 proxy. Wrapped packages can occur anywhere. The log proves only that a blank/special live record was at map coordinate 14 during that snapshot and displaced an ordinary desired record. It does not prove package state belongs to coordinate 14 or that record 205 is a permanent proxy class. Identify the record-to-title/package association and native object behavior before mutating or excluding it.

Consequences: pause extension of RC24's hard-coded DSi aliases and origin-13 compaction. RC22 remains the last architecture-neutral safety milestone (post-sort changes preserved), though its inventory is incomplete. The next development step is a generic read-only visible-object inventory: for every map coordinate and record, log title ID/type, model flags, backing Launcher/SD slot, folder membership, package/wrapped state indicators, and display-name source. Compare that inventory before/after manually moving system icons and before/after wrapping/unwrapping. Sorting should then operate on discovered movable objects and discovered available coordinates, not media-type or coordinate assumptions.

## 2026-08-17 — RC25 generic visible-object inventory

RC25 begins the corrected architecture with a non-mutating inventory routine; existing RC24 sort behavior is otherwise unchanged. During Apply, after the current SD and Launcher sources have been validated and the live model is discovered but before live-map permutation, it writes `/3ds/LumaHome/object-inventory-rc25.csv`.

The inventory covers all 360 top-level coordinates, including empty, duplicate, system, package, folder, SD, NAND, and DSi representations. Each row records coordinate; inline and indirect record index; model-derived 64-bit title ID; words 2–7 and 14; matching SD slot/position/folder; matching Launcher slot/position/folder; and catalog name source/name. Model records and the indirect map are mapped read-only at separate local windows and unmapped before normal live work. The file buffer is 128 KiB with a guarded length limit.

This does not yet decide which objects are movable or remove origin 13. It supplies the evidence needed to derive those rules. A later comparison capture will be required after manually moving a system icon and after a package transition; RC25 provides the first baseline schema.

Visible release `0.1.0-rc25`, runtime `1.10.1`, scan `2.4.0`. Build succeeded. Deployment: `H:\luma\payloads\LumaHome010RC25.firm`; SHA-256 `35DF61C9E88E92AB2DDABA985379E174514B7FFF4326D965B0A5AF4F2295B47D`. RC24 archived as `LumaHome010RC24-DSI-ALIASES.firm`; root firmware unchanged.

Next test: boot RC25 and confirm version. Apply once with the remembered settings solely to generate the baseline inventory; stop after one Apply and reinsert the card. Do not manually rearrange before this first baseline. Inspect the CSV for object classes, coordinate ownership, the two DSi entries, folder record, and blank/package records before choosing the safest method for a second comparison capture.

## 2026-08-18 — RC25 baseline findings; RC26 layered inventory and ID lookup

RC25 produced a 45,229-byte, 360-coordinate inventory. It proves coordinates 0–12 contain ordinary Launcher-backed system titles with catalog names and explicit Launcher positions; they are not semantically fixed by coordinate. Coordinate 1 was a blank record in this snapshot. It also shows inline and indirect layers can name different records at the same coordinate (for example coordinate 13 inline title versus indirect folder), so RC25's single preferred identity was insufficient.

RC25 shutdown failed `ffffff8c` (`-116`) after the folder write. The targeted DSi plan cached Launcher slots from Apply, but HOME had changed the slot arrangement before shutdown. This disproves stable-slot identity. RC26 searches all 360 current Launcher title-ID entries at shutdown, requires exactly one exact 64-bit ID match, and derives the position offset from that current slot. The plan's old slot is diagnostic only. It still writes and verifies only two position bytes.

RC26 inventory format 2 retains the original coordinate summary and appends 720 layered rows: one inline and one indirect row for every coordinate. Each layered row independently resolves its record ID, model words 4/14, SD and Launcher backing slot/position/folder, and catalog name. Output is `/3ds/LumaHome/object-inventory-rc26.csv`. This prevents folder, package, system, and title identities from being conflated when layers diverge.

Visible release `0.1.0-rc26`, runtime `1.10.2`, scan `2.4.1`. Build succeeded. Deployment: `H:\luma\payloads\LumaHome010RC26.firm`; SHA-256 `661C8D934F41F6D33A9DA78790919E0CD150B37EE8A960E8CC337470C8263E8B`. RC25 archived as `LumaHome010RC25-INVENTORY-V1.firm`; root firmware unchanged.

Next test: boot RC26, confirm version, and Apply once without a manual rearrangement. Reinsert after normal power-off/reboot. Required checks: targeted Launcher commit should verify folder + both DSi position writes with result 0, and inventory v2 should contain complete inline/indirect layered sections. Do not perform the system-icon movement comparison until this corrected baseline is inspected.

## 2026-08-18 — RC26 baseline clean; RC27 true read-only capture command

RC26 validation passed: targeted Launcher commit wrote/verified one folder and two DSi titles, result 0; shutdown merge matched 173 with 173 position writes, result 0; persistent SD audit has zero byte and position mismatches. Inventory v2 is 106,224 bytes with 360 inline and 360 indirect rows.

Baseline findings: desired model has 169 top-level and six folder records with no missing full-model identity. Inline update remains partial (168 changes), while indirect needed zero changes after reboot. TWiLight and its booter are correctly present in the indirect layer at coordinates 172/173, but stale inline copies occur at 187/15. Across 360 coordinates, 175 inline/indirect record indices differ. This confirms comparison capture must not invoke sorting.

RC27 adds X `CAPTURE` to the overlay. It opens and atomically pauses HOME, reads/validates current SD and Launcher layouts, constructs a read-only identity grid including all currently positioned Launcher titles, enables probe-only mode, and runs the inventory/scan without any inline, indirect, folder, SaveData, or Launcher mutation. The panel reports `INVENTORY SAVED / NO SORT APPLIED`. A remains the existing mutating Apply command.

RC27 writes `/3ds/LumaHome/object-inventory-rc27.csv` and the normal RC27 live-map report. Visible release `0.1.0-rc27`, runtime `1.10.3`, scan `2.4.2`. Build succeeded. Deployment: `H:\luma\payloads\LumaHome010RC27.firm`; SHA-256 `EEC008B16945EBB6DC1C8FC248A2C9A6FB358723925F009B1BCF3737B6C23060`. RC26 archived as `LumaHome010RC26-LAYERED-BASELINE.firm`; root firmware unchanged.

Next comparison test: boot RC27. Manually move one recognizable Launcher-backed system icon (for example Camera) to an obviously different coordinate without sorting. Open overlay and press X, not A. Confirm `INVENTORY SAVED / NO SORT APPLIED`, then reinsert the card without applying a sort. Compare RC27 layered rows and backing positions against the RC26 baseline to identify which maps/backing fields HOME changes for a manual system-icon move.

## 2026-08-18 — RC27 capture failed on locked Launcher; RC28 resident fallback

The user moved the test icon and pressed X, but capture failed. Runtime recorded exact result `c92044e7`; no RC27 inventory was created and no crash occurred. The failure was opening `Launcher.dat` while HOME owned the archive. This read-only path had omitted the resident-Launcher fallback already proven in Apply.

RC28 first attempts the validated file read. If HOME has it locked, capture calls the existing Launcher runtime discovery, maps the discovered region read-only, reconstructs file alignment by copying the resident object to offset +8, unmaps it, and validates the result before inventory. HOME remains paused for a consistent snapshot and probe-only mode still prevents model mutations.

`/3ds/LumaHome/capture-report-rc28.txt` now records file result, resident discovery/map result, address, match count, unlock result, and final result, so another capture failure is self-localizing. Inventory/live-map filenames and visible versions are RC28.

Visible release `0.1.0-rc28`, runtime `1.10.4`, scan `2.4.3`. Build succeeded. Deployment: `H:\luma\payloads\LumaHome010RC28.firm`; SHA-256 `254EA5B2D5B8CE585ED1D266DA575E0187BAB81679C574C0295370F1418E4AFF`. RC27 archived as `LumaHome010RC27-CAPTURE-NO-LAUNCHER-FALLBACK.firm`; root firmware unchanged.

Next test: leave the manually moved system icon where it is, boot RC28, open overlay, press X only, and verify `INVENTORY SAVED / NO SORT APPLIED`. Reinsert immediately. Do not Apply or move another object; the comparison must isolate the single existing manual move against RC26.

## 2026-08-18 — RC28 false-success zero-address scan; RC29 SD discovery

RC28 reported capture success and its Launcher fallback succeeded (`file=c92044e7`, resident result 0, address `346bf208`, five matches), but no object inventory was created. The live-map report exposed `raw=0`, `processed=0`: capture had not run SD runtime discovery. The scanner then compared every memory word against zero, logged 96 false raw references in the first 4 KiB, hit its reference cap, and ended with `icon_owner=0`, `icon_model=0`. RC28 incorrectly treated that as success.

RC29 runs the existing validated `DiscoverSdRuntime` before Launcher discovery, records the actual raw/processed addresses in the globals used by the scan, and adds defensive nonzero guards to raw/grid reference comparisons. After scan, `iconOwner==0` is failure `-123`; success can no longer be reported without reaching the icon model/inventory path.

The RC29 capture report adds SD discovery result and discovered raw/processed addresses. Output filenames and visible versions are RC29. No sort behavior change.

Visible release `0.1.0-rc29`, runtime `1.10.5`, scan `2.4.4`. Build succeeded. Deployment: `H:\luma\payloads\LumaHome010RC29.firm`; SHA-256 `B4446D254ABAFACEA401091CE321E4F2891174B0AFC07938DB4C95027602606C`. RC28 archived as `LumaHome010RC28-ZERO-ADDRESS-SCAN.firm`; root firmware unchanged.

Next test remains isolated: leave the manually moved system icon unchanged, boot RC29, press X only, confirm inventory saved/no sort, and reinsert. A valid result requires `capture-report-rc29.txt` result 0 with nonzero raw/processed addresses and `object-inventory-rc29.csv` present.

## 2026-08-18 — RC29 system-icon movement comparison succeeded

The RC29 X capture is valid and read-only. `capture-report-rc29.txt` reports SD discovery result 0, raw `346ca1e0`, processed `346ccf90`, resident Launcher address `346bf208` with five matches, and final result 0. The layered inventory contains all 720 rows.

The user manually moved Nintendo 3DS Camera (`0004001000021400`). Against the clean RC26 baseline, its authoritative indirect-layer coordinate and Launcher position both changed from 2 to 222. This directly proves that coordinate 2—and therefore the observed 0–12 system-icon area—is not fixed or protected. The generic sorter must include these objects based on identity/semantics, not impose a coordinate-origin cutoff.

The comparison also clarifies the two live layers. The indirect layer changed only the Camera from coordinate 2 to 222 (plus already-known unrelated model evolution), while the inline layer showed a packed/stale cascade: Mii Maker and following system rows shifted toward the vacancy, Camera appeared in an inline copy at coordinate 15, and stale title copies existed later in the array. Launcher slot numbers also changed as entries were reconstructed, while the exact title-ID lookup reliably found Camera with position 222. Therefore neither inline record index nor Launcher slot is stable identity. Exact 64-bit title ID plus current backing lookup is the supported identity path; the indirect coordinate is the cleanest observed persistent placement signal. Inline data remains necessary for immediate display but must be rebuilt from identities rather than treated as an authoritative one-row-per-icon inventory.

No firmware or sorting behavior changed in this iteration. RC29 remains installed. The next evidence gap is package/wrapped-state ownership: capture the same layered inventory immediately before and after one known title changes between wrapped and unwrapped, without sorting or manual movement between captures. Preserve both snapshots under distinct filenames before implementing generic collapse/live permutation, because wrapped packages may occupy any coordinate and no coordinate-specific proxy assumption is allowed.

## 2026-08-18 — RC30 removes the fixed coordinate origin

The user explicitly deferred wrapped/unwrapped investigation because all current titles are unwrapped. RC30 advances the generic movable-object path without changing package behavior.

RC29 proved Nintendo 3DS Camera moved normally from coordinate 2 to 222 and remained identifiable by exact title ID in current Launcher state. RC30 therefore removes the NAND eligibility check that admitted only the two experimental hard-coded DSi IDs. Every request entry—SD or Launcher-backed—is now eligible only when its exact 64-bit title ID has exactly one current backing match and its current folder/position is valid. This includes positioned system icons below coordinate 13. Ambiguous, absent, invalid, and non-title objects remain excluded by the existing validation rather than by product ID or coordinate.

Top-level Collapse now starts at coordinate 0 instead of 13. The combined title/folder capacity check, folder Before/After compact placement, and missing-folder recovery lower bound were changed consistently to origin 0. Folder-internal compaction was already origin 0. Non-collapse mode still repartitions only the coordinates actually occupied by discovered titles and folders, so it creates no new coordinates. The existing DSi aliases remain temporarily for display names, but no longer control whether other Launcher titles are sortable. Hook installation, L+Y input, resume repair, live permutation, targeted persistence, RC22 shutdown merge protection, and package logic are unchanged.

Visible release `0.1.0-rc30`, runtime `1.10.6`, scan `2.5.0`. Build succeeded. Nested LumaHome commit `60f0f56` was pushed to `lumahome/home-menu-framework`. Deployment: `H:\luma\payloads\LumaHome010RC30.firm`; SHA-256 `F25621E5A58136954785B2433DB3816E5F65D8F3E2385716664AE6E8AA9BF3DA`. RC29 archived as `LumaHome010RC29-SYSTEM-MOVE-CAPTURE.firm`; root `H:\boot.firm` remains SHA-256 `10A8356230FF4C3E7D72FCFBC2F7E47CC12717DE2B6AF5122E081B51E023CC2A`.

Focused RC30 test: boot RC30 and verify the visible version. Open L+Y; select A-Z, folder Before, Collapse ON, and the currently preferred traversal, then Apply exactly once. The manually displaced Camera at coordinate 222 should join the alphabetical result rather than remain outside it, system icons should be allowed to occupy/sort through the beginning of the grid, and the folder should remain first. Confirm HOME remains responsive and note whether the live display is complete. Normal power-off/reboot and verify the same placement persists. Reinsert afterward; inspect the RC30 transaction/live-map and shutdown logs before any second sort.
