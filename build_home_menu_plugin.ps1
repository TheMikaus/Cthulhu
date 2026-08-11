$ErrorActionPreference = "Stop"

$repository = $PSScriptRoot
$plugin = Join-Path $repository "runtime\home_menu_plugin"
$output = Join-Path $repository "runtime\out"

docker build --tag cthulhu-home-menu-plugin-toolchain:2 $plugin
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

docker run --rm `
    --volume "${plugin}:/project" `
    --workdir /project `
    cthulhu-home-menu-plugin-toolchain:2 `
    make
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

New-Item -ItemType Directory -Force -Path $output | Out-Null
Copy-Item -LiteralPath (Join-Path $plugin "CthulhuHomeMenu.3gx") `
    -Destination (Join-Path $output "CthulhuHomeMenu.3gx") -Force
