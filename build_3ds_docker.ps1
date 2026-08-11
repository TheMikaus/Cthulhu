$ErrorActionPreference = "Stop"

$repository = $PSScriptRoot
docker run --rm `
    --volume "${repository}:/project" `
    --workdir /project `
    devkitpro/devkitarm@sha256:116afba8df8453961de2936ffab20dd441edf4d682856c1ec8b0e53d7ed0bbf5 `
    make -j2

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
