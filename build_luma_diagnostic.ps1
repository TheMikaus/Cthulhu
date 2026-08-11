$ErrorActionPreference = "Stop"

$repository = $PSScriptRoot
$runtime = Join-Path $repository "runtime\luma"
$buildRoot = Join-Path $repository "runtime\.build"
$lumaSource = Join-Path $buildRoot "Luma3DS"
$output = Join-Path $repository "runtime\out"
$lumaCommit = "d30ac8d1c665ed2a50dc30b291f7eb6b33e9890a"

docker build --tag cthulhu-luma-toolchain:1 $runtime
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if (Test-Path -LiteralPath $lumaSource) {
    throw "Build directory already exists: $lumaSource. Remove runtime/.build to rebuild from scratch."
}

New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
git clone https://github.com/LumaTeam/Luma3DS.git $lumaSource
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
git -C $lumaSource checkout --detach $lumaCommit
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Copy-Item -LiteralPath (Join-Path $runtime "source\home_menu_diagnostics.c") `
    -Destination (Join-Path $lumaSource "sysmodules\rosalina\source\menus\home_menu_diagnostics.c")
Copy-Item -LiteralPath (Join-Path $runtime "source\home_menu_diagnostics.h") `
    -Destination (Join-Path $lumaSource "sysmodules\rosalina\include\menus\home_menu_diagnostics.h")
Copy-Item -LiteralPath (Join-Path $runtime "source\cthulhu_postboot_plugin.c") `
    -Destination (Join-Path $lumaSource "sysmodules\rosalina\source\plugin\cthulhu_postboot_plugin.c")
Copy-Item -LiteralPath (Join-Path $runtime "source\cthulhu_postboot_plugin.h") `
    -Destination (Join-Path $lumaSource "sysmodules\rosalina\include\plugin\cthulhu_postboot_plugin.h")
Copy-Item -LiteralPath (Join-Path $runtime "source\cthulhu_postboot_trampoline.s") `
    -Destination (Join-Path $lumaSource "sysmodules\rosalina\source\plugin\cthulhu_postboot_trampoline.s")
Copy-Item -LiteralPath (Join-Path $runtime "source\cthulhu_home_static_patch.c") `
    -Destination (Join-Path $lumaSource "sysmodules\loader\source\cthulhu_home_static_patch.c")
Copy-Item -LiteralPath (Join-Path $runtime "source\cthulhu_home_static_patch.h") `
    -Destination (Join-Path $lumaSource "sysmodules\loader\source\cthulhu_home_static_patch.h")
Copy-Item -LiteralPath (Join-Path $runtime "source\cthulhu_home_static_stub.s") `
    -Destination (Join-Path $lumaSource "sysmodules\loader\source\cthulhu_home_static_stub.s")
Copy-Item -LiteralPath (Join-Path $runtime "source\cthulhu_runtime_logger.c") `
    -Destination (Join-Path $lumaSource "sysmodules\rosalina\source\cthulhu_runtime_logger.c")
Copy-Item -LiteralPath (Join-Path $runtime "source\cthulhu_runtime_logger.h") `
    -Destination (Join-Path $lumaSource "sysmodules\rosalina\include\cthulhu_runtime_logger.h")
git -C $lumaSource apply (Join-Path $runtime "rosalina-menu.patch")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
git -C $lumaSource apply (Join-Path $runtime "home-static-loader.patch")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
docker run --rm `
    --volume "${lumaSource}:/project" `
    --workdir /project `
    cthulhu-luma-toolchain:1 `
    make -j2
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

New-Item -ItemType Directory -Force -Path $output | Out-Null
Copy-Item -LiteralPath (Join-Path $lumaSource "boot.firm") `
    -Destination (Join-Path $output "CthulhuHomeOSD143.firm")
