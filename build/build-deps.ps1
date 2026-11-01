<#
.SYNOPSIS
  Builds the third-party dependencies of Shawky Call (x64, static CRT /MT):
  vcpkg (opus, openssl, sqlitecpp), bcg729 and pjproject 2.15.1.

.DESCRIPTION
  Layout produced under -Work:
    vcpkg\               pinned vcpkg checkout
    deps\include, lib    headers and .lib files used by pjproject and MicroSIP
    pjproject\           pjproject 2.15.1, built (lib\libpjproject-x86_64-x64-vc14-Release-Static.lib)

  Requires: Visual Studio 2022 (MSVC v143, C++ MFC/ATL component), CMake, Git.
  Run from a "Developer PowerShell for VS 2022" or let build.ps1 call it.
#>
param(
    [string]$Work = (Join-Path $PSScriptRoot "..\_work")
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

# ---- Pinned versions (reproducible builds) --------------------------------
$PjTag       = "2.15.1"
$PjCommit    = "7de6e686fee1642f5443a3f39f0e5ad5e478a117"
$VcpkgTag    = "2026.07.29"
$VcpkgCommit = "9e593bb18ea69cc5095e012465dcd675a822ed0d"
$Bcg729Tag   = "1.1.1"
$Bcg729Commit = "faaa895862165acde6df8add722ba4f85a25007d"
$Triplet     = "x64-windows-static"
$Toolset     = "v143"
# ---------------------------------------------------------------------------

function Invoke-Checked([string]$Exe, [string[]]$Arguments) {
    Write-Host ">> $Exe $($Arguments -join ' ')"
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Exe failed with exit code $LASTEXITCODE" }
}

function Get-PinnedRepo([string]$Url, [string]$Tag, [string]$Commit, [string]$Dir) {
    if (-not (Test-Path (Join-Path $Dir ".git"))) {
        Invoke-Checked git @("clone", "--quiet", "--depth", "1", "--branch", $Tag, $Url, $Dir)
    }
    $head = (& git -C $Dir rev-parse HEAD).Trim()
    if ($head -ne $Commit) { throw "$Url at $Tag is $head, expected $Commit" }
}

$Work = [System.IO.Path]::GetFullPath($Work)
New-Item -ItemType Directory -Force -Path $Work | Out-Null
$Deps = Join-Path $Work "deps"
New-Item -ItemType Directory -Force -Path (Join-Path $Deps "include"), (Join-Path $Deps "lib") | Out-Null

# ---- vcpkg: opus, openssl, sqlitecpp (+ sqlite3) ---------------------------
$Vcpkg = Join-Path $Work "vcpkg"
Get-PinnedRepo "https://github.com/microsoft/vcpkg.git" $VcpkgTag $VcpkgCommit $Vcpkg
if (-not (Test-Path (Join-Path $Vcpkg "vcpkg.exe"))) {
    Invoke-Checked (Join-Path $Vcpkg "bootstrap-vcpkg.bat") @("-disableMetrics")
}
if (-not $env:VCPKG_DEFAULT_BINARY_CACHE) {
    $env:VCPKG_DEFAULT_BINARY_CACHE = Join-Path $Work "vcpkg-cache"
}
New-Item -ItemType Directory -Force -Path $env:VCPKG_DEFAULT_BINARY_CACHE | Out-Null
Invoke-Checked (Join-Path $Vcpkg "vcpkg.exe") @("install", "--triplet", $Triplet, "--clean-after-build",
    "opus", "openssl", "sqlitecpp")
$Installed = Join-Path $Vcpkg "installed\$Triplet"
Copy-Item -Recurse -Force (Join-Path $Installed "include\*") (Join-Path $Deps "include")
Copy-Item -Force (Join-Path $Installed "lib\*.lib") (Join-Path $Deps "lib")
# pjproject's opus.c links "libopus.a" on MSVC (#pragma comment); vcpkg names it opus.lib.
Copy-Item -Force (Join-Path $Deps "lib\opus.lib") (Join-Path $Deps "lib\libopus.a")

# ---- bcg729 (G.729) --------------------------------------------------------
$Bcg = Join-Path $Work "bcg729"
Get-PinnedRepo "https://github.com/BelledonneCommunications/bcg729.git" $Bcg729Tag $Bcg729Commit $Bcg
$BcgBuild = Join-Path $Work "bcg729-build"
Invoke-Checked cmake @("-S", $Bcg, "-B", $BcgBuild, "-G", "Visual Studio 17 2022", "-A", "x64",
    "-DENABLE_SHARED=OFF", "-DENABLE_STATIC=ON", "-DENABLE_TESTS=OFF",
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
    "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW", "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded",
    "-DCMAKE_INSTALL_PREFIX=$Deps")
Invoke-Checked cmake @("--build", $BcgBuild, "--config", "Release", "--target", "install")
if (-not (Test-Path (Join-Path $Deps "lib\bcg729.lib"))) {
    $lib = Get-ChildItem (Join-Path $Deps "lib") -Filter "*bcg729*.lib" | Select-Object -First 1
    if (-not $lib) { throw "bcg729 library not found after install" }
    Copy-Item $lib.FullName (Join-Path $Deps "lib\bcg729.lib")
}

# ---- pjproject 2.15.1 ------------------------------------------------------
$Pj = Join-Path $Work "pjproject"
Get-PinnedRepo "https://github.com/pjsip/pjproject.git" $PjTag $PjCommit $Pj
Copy-Item -Force (Join-Path $PSScriptRoot "config_site.h") (Join-Path $Pj "pjlib\include\pj\config_site.h")

# Extra include/lib paths for cl.exe and link.exe (honored by MSBuild).
# BCG729_STATIC: bcg729 headers default to __declspec(dllimport); we link the static lib.
$env:CL = "/I`"$Deps\include`" /DBCG729_STATIC $env:CL"
$env:LINK = "/LIBPATH:`"$Deps\lib`" $env:LINK"

Invoke-Checked msbuild @((Join-Path $Pj "pjproject-vs14.sln"), "/t:libpjproject", "/m", "/nologo", "/v:minimal",
    "/p:Configuration=Release-Static", "/p:Platform=x64", "/p:BuildToolset=$Toolset")

$PjLib = Join-Path $Pj "lib\libpjproject-x86_64-x64-vc14-Release-Static.lib"
if (-not (Test-Path $PjLib)) { throw "pjproject library not found: $PjLib" }
Write-Host "Dependencies ready in $Work"
