$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$engineRoot = 'C:\Program Files\Epic Games\UE_5.8'
# MSVC response files can misread Cyrillic paths; retain the original project
# and use an ASCII junction for build outputs and source paths.
$buildAlias = Join-Path $env:TEMP 'PogonyaUE58'
if (!(Test-Path -LiteralPath $buildAlias)) {
    New-Item -ItemType Junction -Path $buildAlias -Target $projectRoot | Out-Null
}
$resolvedLink = Get-Item -LiteralPath $buildAlias
if ($resolvedLink.Target -ne $projectRoot) { throw "Build alias belongs to another project: $buildAlias" }
& "$engineRoot\Engine\Build\BatchFiles\Build.bat" PogonyaEditor Win64 Development "-Project=$buildAlias\Pogonya.uproject" -NoUBA -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw 'Unreal build failed; see UnrealBuildTool log.' }
