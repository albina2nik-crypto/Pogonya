$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$engine = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$projectPath = Join-Path $projectRoot 'Pogonya.uproject'
$scriptPath = Join-Path $PSScriptRoot 'open_blueprint_game.py'
$launchArgs = '"' + $projectPath + '" -ExecutePythonScript="' + $scriptPath + '"'
# The installed GPU driver has already rendered the game successfully. Suppress
# only its startup reminder for this launch; do not change project/system settings.
$launchArgs += ' -ini:Engine:[ConsoleVariables]:r.WarnOfBadDrivers=0'
# This is an interactive editor explicitly opened for the user to inspect graphs.
Start-Process -FilePath $engine -ArgumentList $launchArgs -WindowStyle Normal
