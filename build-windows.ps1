# ChordMate - compilazione e installazione su Windows (VST3)
# Uso (PowerShell, dalla cartella del progetto):
#   powershell -ExecutionPolicy Bypass -File .\build-windows.ps1
# Opzioni:  -SkipTests   salta i test del motore
#           -InstallDir  cartella di destinazione del .vst3 (default: automatica)
param(
    [switch]$SkipTests,
    [string]$InstallDir = ""
)

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

function Step($t) { Write-Host "`n=== $t ===" -ForegroundColor Cyan }
function Fail($t) { Write-Host "`nERRORE: $t" -ForegroundColor Red; exit 1 }

# ------------------------------------------------------------------ 1. prerequisiti
Step "Controllo prerequisiti"
$missing = @()

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { $missing += "winget install Kitware.CMake" }
if (-not (Get-Command git   -ErrorAction SilentlyContinue)) { $missing += "winget install Git.Git" }

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
$hasVS = $false
if (Test-Path $vswhere) {
    $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($vs) { $hasVS = $true; Write-Host "Compilatore C++ trovato: $vs" }
}
if (-not $hasVS) {
    $missing += 'winget install Microsoft.VisualStudio.2022.BuildTools --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"'
}

if ($missing.Count -gt 0) {
    Write-Host "Manca qualcosa. Esegui questi comandi, poi CHIUDI e RIAPRI PowerShell e rilancia lo script:" -ForegroundColor Yellow
    $missing | ForEach-Object { Write-Host "  $_" }
    exit 1
}

if ($PSScriptRoot.Length -gt 40) {
    Write-Host "Attenzione: il percorso del progetto e' lungo ($($PSScriptRoot.Length) caratteri)." -ForegroundColor Yellow
    Write-Host "Se la build fallisce con errori di percorso, sposta la cartella in C:\dev\ChordMate." -ForegroundColor Yellow
}

# ------------------------------------------------------------------ 2. configura e compila
Step "Configurazione (la prima volta scarica JUCE da GitHub, serve internet)"
cmake -B build -A x64
if ($LASTEXITCODE -ne 0) { Fail "configurazione CMake fallita (vedi messaggi sopra)." }

Step "Compilazione (la prima volta 5-15 minuti)"
cmake --build build --config Release --parallel
if ($LASTEXITCODE -ne 0) { Fail "compilazione fallita (vedi messaggi sopra)." }

if (-not $SkipTests) {
    Step "Test del motore di teoria"
    ctest --test-dir build -C Release --output-on-failure
    if ($LASTEXITCODE -ne 0) { Fail "i test non passano: non installo il plugin." }
}

# ------------------------------------------------------------------ 3. installazione
Step "Installazione"
$bundle = Get-ChildItem -Path build -Recurse -Directory -Filter "ChordMate.vst3" | Select-Object -First 1
if (-not $bundle) { Fail "non trovo ChordMate.vst3 nella cartella build." }

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
           ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if ($InstallDir -eq "") {
    if ($isAdmin) { $InstallDir = Join-Path $env:CommonProgramFiles "VST3" }
    else          { $InstallDir = Join-Path $env:LOCALAPPDATA "Programs\Common\VST3" }
}
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
$dest = Join-Path $InstallDir "ChordMate.vst3"
if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }
Copy-Item -Recurse -Force $bundle.FullName $dest

Write-Host "`nFATTO. Plugin installato in:" -ForegroundColor Green
Write-Host "  $dest"
Write-Host "`nIn Ableton Live 12: Impostazioni > Plug-in > attiva 'Usa le cartelle di sistema VST3'"
Write-Host "e premi 'Riesamina' (tieni premuto Alt per la scansione completa)."
if ($InstallDir -notlike "$env:CommonProgramFiles*") {
    Write-Host "Se Live non lo vede, imposta come cartella VST3 personalizzata: $InstallDir" -ForegroundColor Yellow
}
