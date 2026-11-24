param([switch]$HostGame, [string]$Join = '')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$engine = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$map = '/Game/Pogonya/Maps/L_Sector04'
if ($HostGame) { $map += '?listen' }
if ($Join) { $map = $Join }
& $engine "$projectRoot\Pogonya.uproject" $map -game -windowed -ResX=1280 -ResY=720 -log
