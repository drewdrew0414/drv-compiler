#Requires -Version 5.1
<#
.SYNOPSIS
    dri language compiler Windows installer script
.DESCRIPTION
    Downloads dri.exe from drvpm-registry.cloud, registers PATH,
    installs VSCode extension, and registers .dri file association.
    Supports Windows x64 and ARM64. Includes local build fallback.
.PARAMETER InstallDir
    Installation directory (default: %LOCALAPPDATA%\dri)
.PARAMETER NoVSCode
    Skip VSCode extension installation
.PARAMETER Uninstall
    Remove dri compiler and all associated settings
.PARAMETER Silent
    Non-interactive mode — no prompts, use all defaults
.PARAMETER Verify
    Check current installation status and run dri --version
#>
param(
    [string]$InstallDir = "$env:LOCALAPPDATA\dri",
    [switch]$NoVSCode,
    [switch]$Uninstall,
    [switch]$Silent,
    [switch]$Verify
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ── Force TLS 1.2 / 1.3 at script start ────────────────────────────────────
[System.Net.ServicePointManager]::SecurityProtocol =
    [System.Net.SecurityProtocolType]::Tls12 -bor
    [System.Net.SecurityProtocolType]::Tls13

# ── Constants ────────────────────────────────────────────────────────────────
$VERSION       = "0.1.0"
$RELEASE_URL   = "https://drvpm-registry.cloud/dist/v0.1.0/window"
$VSIX_NAME     = "dri-lang-$VERSION.vsix"
$REG_PATH      = "HKCU:\Software\dri-lang"

# ── ARM64 detection ──────────────────────────────────────────────────────────
$arch = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture
$isArm64 = ($arch.ToString() -eq "Arm64")
$COMPILER_NAME = if ($isArm64) { "dri-arm64.exe" } else { "dri.exe" }
$COMPILER_INSTALL_NAME = "dri.exe"   # always installed as dri.exe in target dir

# ── Console output helpers ───────────────────────────────────────────────────
function Write-Step  { param($msg) Write-Host "  >> $msg" -ForegroundColor Cyan }
function Write-OK    { param($msg) Write-Host "  OK $msg" -ForegroundColor Green }
function Write-Warn  { param($msg) Write-Host "  !! $msg" -ForegroundColor Yellow }
function Write-Err   { param($msg) Write-Host "  XX $msg" -ForegroundColor Red }

function Show-Banner {
    Write-Host ""
    Write-Host "  +======================================+" -ForegroundColor DarkCyan
    Write-Host "  |   dri Language Compiler v$VERSION      |" -ForegroundColor DarkCyan
    Write-Host "  |   HPC Systems Language               |" -ForegroundColor DarkCyan
    if ($isArm64) {
    Write-Host "  |   Platform: Windows ARM64            |" -ForegroundColor DarkCyan
    } else {
    Write-Host "  |   Platform: Windows x64              |" -ForegroundColor DarkCyan
    }
    Write-Host "  +======================================+" -ForegroundColor DarkCyan
    Write-Host ""
}

# ── -Verify flag: check install and run dri --version ────────────────────────
function Invoke-Verify {
    Show-Banner
    Write-Step "Verifying dri installation..."

    $compilerPath = Join-Path $InstallDir "dri.exe"

    if (-not (Test-Path $compilerPath)) {
        Write-Err "dri.exe not found at: $compilerPath"
        Write-Warn "Run the installer first."
        exit 1
    }

    Write-OK "Binary found: $compilerPath"

    # Check PATH
    $userPath = [Environment]::GetEnvironmentVariable("PATH", "User")
    $machinePath = [Environment]::GetEnvironmentVariable("PATH", "Machine")
    if (($userPath -like "*$InstallDir*") -or ($machinePath -like "*$InstallDir*")) {
        Write-OK "InstallDir is in PATH"
    } else {
        Write-Warn "InstallDir is NOT in PATH: $InstallDir"
    }

    # Check registry
    if (Test-Path $REG_PATH) {
        $regVersion = (Get-ItemProperty $REG_PATH -ErrorAction SilentlyContinue).Version
        Write-OK "Registry entry found, Version: $regVersion"
    } else {
        Write-Warn "Registry entry not found at $REG_PATH"
    }

    # Run dri --version
    Write-Step "Running: dri --version"
    try {
        $ver = & $compilerPath "--version" 2>&1
        Write-OK "dri --version output: $ver"
    } catch {
        Write-Warn "Could not run dri.exe: $($_.Exception.Message)"
    }

    Write-Host ""
}

# ── Uninstall ────────────────────────────────────────────────────────────────
function Invoke-Uninstall {
    Write-Step "Removing dri compiler..."

    foreach ($scope in @("Machine", "User")) {
        $p = [Environment]::GetEnvironmentVariable("PATH", $scope)
        if ($p -like "*$InstallDir*") {
            $newPath = ($p.Split(";") | Where-Object { $_ -ne $InstallDir }) -join ";"
            try {
                [Environment]::SetEnvironmentVariable("PATH", $newPath, $scope)
                Write-OK "Removed from $scope PATH"
            } catch {
                Write-Warn "Could not remove from $scope PATH: $($_.Exception.Message)"
            }
        }
    }

    if (Test-Path $InstallDir) {
        Remove-Item $InstallDir -Recurse -Force
        Write-OK "Installation directory removed: $InstallDir"
    }

    if (Test-Path $REG_PATH) {
        Remove-Item $REG_PATH -Recurse -Force
        Write-OK "Registry entry removed"
    }

    # Remove .dri file association
    try {
        if (Test-Path "HKCU:\Software\Classes\.dri")       { Remove-Item "HKCU:\Software\Classes\.dri"       -Recurse -Force }
        if (Test-Path "HKCU:\Software\Classes\dri_source") { Remove-Item "HKCU:\Software\Classes\dri_source" -Recurse -Force }
        Write-OK "File association removed"
    } catch {
        Write-Warn "File association removal skipped: $($_.Exception.Message)"
    }

    Write-Host ""
    Write-OK "dri uninstalled successfully"
}

# ── OpenMP / libomp guide ─────────────────────────────────────────────────────
function Show-OpenMPGuide {
    Write-Host ""
    Write-Host "  OpenMP / libomp Setup:" -ForegroundColor Cyan
    Write-Host "    Windows : winget install LLVM.LLVM  (libomp is included)" -ForegroundColor White
    Write-Host "    macOS   : brew install libomp && brew install llvm" -ForegroundColor White
    Write-Host "    Linux   : sudo apt-get install libomp-dev  (Debian/Ubuntu)" -ForegroundColor White
    Write-Host "              sudo dnf install libomp-devel    (Fedora/RHEL)" -ForegroundColor White
    Write-Host ""
}

# ── Install build tools (LLVM / CMake / MinGW) ───────────────────────────────
function Install-BuildTools {

    $needCxx   = -not (Get-Command "clang++" -ErrorAction SilentlyContinue) -and
                 -not (Get-Command "g++"     -ErrorAction SilentlyContinue)
    $needCmake = -not (Get-Command "cmake"   -ErrorAction SilentlyContinue)

    if (-not $needCxx -and -not $needCmake) {
        Write-OK "C++ compiler and CMake already present"
        return
    }

    Write-Host ""
    Write-Step "Missing build tools detected — attempting auto-install..."

    # ── 1. Try winget: LLVM then CMake ───────────────────────────────────────
    $hasWinget = Get-Command "winget" -ErrorAction SilentlyContinue
    if ($hasWinget) {
        Write-Step "Package manager: winget"

        if ($needCxx) {
            Write-Step "Installing LLVM (clang++ + libomp) via winget..."
            try {
                winget install --id LLVM.LLVM -e --silent --accept-package-agreements --accept-source-agreements
                $llvmBin = "$env:ProgramFiles\LLVM\bin"
                if (Test-Path $llvmBin) { $env:PATH = "$env:PATH;$llvmBin" }
                if (Get-Command "clang++" -ErrorAction SilentlyContinue) {
                    Write-OK "clang++ installed (includes libomp)"
                    $needCxx = $false
                } else {
                    Write-Warn "LLVM installed — open a new terminal for clang++ to take effect"
                    $needCxx = $false
                }
            } catch {
                Write-Warn "LLVM winget install failed: $($_.Exception.Message)"
            }
        }

        if ($needCmake) {
            Write-Step "Installing CMake via winget..."
            try {
                winget install --id Kitware.CMake -e --silent --accept-package-agreements --accept-source-agreements
                $cmakeBin = "$env:ProgramFiles\CMake\bin"
                if (Test-Path $cmakeBin) { $env:PATH = "$env:PATH;$cmakeBin" }
                if (Get-Command "cmake" -ErrorAction SilentlyContinue) {
                    Write-OK "cmake installed"
                    $needCmake = $false
                } else {
                    Write-Warn "cmake installed — open a new terminal for cmake to take effect"
                    $needCmake = $false
                }
            } catch {
                Write-Warn "CMake winget install failed: $($_.Exception.Message)"
            }
        }
    }

    # ── 2. Try Scoop: llvm cmake ─────────────────────────────────────────────
    if ($needCxx -or $needCmake) {
        $hasScoop = Get-Command "scoop" -ErrorAction SilentlyContinue
        if ($hasScoop) {
            Write-Step "Package manager: scoop"

            if ($needCxx) {
                Write-Step "Installing LLVM via scoop..."
                try {
                    scoop install llvm
                    if (Get-Command "clang++" -ErrorAction SilentlyContinue) {
                        Write-OK "clang++ installed via scoop"
                        $needCxx = $false
                    } else {
                        Write-Warn "scoop llvm installed — open a new terminal to use clang++"
                        $needCxx = $false
                    }
                } catch {
                    Write-Warn "scoop llvm failed: $($_.Exception.Message)"
                }
            }

            if ($needCmake) {
                Write-Step "Installing cmake via scoop..."
                try {
                    scoop install cmake
                    if (Get-Command "cmake" -ErrorAction SilentlyContinue) {
                        Write-OK "cmake installed via scoop"
                        $needCmake = $false
                    } else {
                        Write-Warn "scoop cmake installed — open a new terminal to use cmake"
                        $needCmake = $false
                    }
                } catch {
                    Write-Warn "scoop cmake failed: $($_.Exception.Message)"
                }
            }
        }
    }

    # ── 3. Try Chocolatey: MinGW + CMake ─────────────────────────────────────
    if ($needCxx -or $needCmake) {
        $hasChoco = Get-Command "choco" -ErrorAction SilentlyContinue

        if (-not $hasChoco) {
            Write-Step "winget/scoop unavailable — installing Chocolatey..."
            try {
                Set-ExecutionPolicy Bypass -Scope Process -Force
                [System.Net.ServicePointManager]::SecurityProtocol =
                    [System.Net.SecurityProtocolType]::Tls12 -bor
                    [System.Net.SecurityProtocolType]::Tls13
                Invoke-Expression ((New-Object System.Net.WebClient).DownloadString(
                    'https://community.chocolatey.org/install.ps1'))
                $env:PATH = "$env:PATH;$env:ALLUSERSPROFILE\chocolatey\bin"
                $hasChoco = Get-Command "choco" -ErrorAction SilentlyContinue
                if ($hasChoco) { Write-OK "Chocolatey installed" }
            } catch {
                Write-Warn "Chocolatey install failed: $($_.Exception.Message)"
            }
        }

        if ($hasChoco -or (Get-Command "choco" -ErrorAction SilentlyContinue)) {
            if ($needCxx) {
                Write-Step "Installing MinGW (g++) via Chocolatey..."
                try {
                    choco install mingw -y --no-progress
                    $mingwBin = "$env:SystemDrive\tools\mingw64\bin"
                    if (Test-Path $mingwBin) { $env:PATH = "$env:PATH;$mingwBin" }
                    if (Get-Command "g++" -ErrorAction SilentlyContinue) {
                        Write-OK "g++ installed via Chocolatey"
                        $needCxx = $false
                    } else {
                        Write-Warn "MinGW installed — open a new terminal to use g++"
                        $needCxx = $false
                    }
                } catch {
                    Write-Warn "MinGW Chocolatey install failed: $($_.Exception.Message)"
                }
            }

            if ($needCmake) {
                Write-Step "Installing CMake via Chocolatey..."
                try {
                    choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' -y --no-progress
                    if (Get-Command "cmake" -ErrorAction SilentlyContinue) {
                        Write-OK "cmake installed via Chocolatey"
                        $needCmake = $false
                    } else {
                        Write-Warn "cmake installed — open a new terminal to use cmake"
                        $needCmake = $false
                    }
                } catch {
                    Write-Warn "CMake Chocolatey install failed: $($_.Exception.Message)"
                }
            }
        }
    }

    # ── 4. Manual guide if all auto-install paths failed ─────────────────────
    if ($needCxx) {
        Write-Warn "Could not auto-install C++ compiler. Install manually:"
        Write-Host "    LLVM  : https://github.com/llvm/llvm-project/releases (LLVM-*-win64.exe)" -ForegroundColor Yellow
        Write-Host "    MinGW : https://github.com/niXman/mingw-builds-binaries/releases"         -ForegroundColor Yellow
    }
    if ($needCmake) {
        Write-Warn "Could not auto-install CMake. Install manually:"
        Write-Host "    CMake : https://cmake.org/download/ (cmake-*-windows-x86_64.msi)"         -ForegroundColor Yellow
    }

    Show-OpenMPGuide
}

# ── Dependency check ─────────────────────────────────────────────────────────
function Test-Dependency {
    param([string]$Name, [string]$Command)
    try {
        $null = & $Command "--version" 2>&1
        Write-OK "$Name detected"
        return $true
    } catch {
        Write-Warn "$Name not found — some features may be unavailable"
        return $false
    }
}

# ── SHA-256 checksum verification stub ───────────────────────────────────────
function Test-Checksum {
    param([string]$FilePath, [string]$ExpectedSha256)
    # TODO: populate $ExpectedSha256 values from a checksums.txt served alongside the release
    # Example: https://drvpm-registry.cloud/dist/v0.1.0/window/checksums.sha256
    if ([string]::IsNullOrEmpty($ExpectedSha256)) {
        Write-Warn "Checksum not available — skipping verification (TODO: add checksums.sha256)"
        return $true
    }
    $actual = (Get-FileHash -Path $FilePath -Algorithm SHA256).Hash
    if ($actual -ieq $ExpectedSha256) {
        Write-OK "SHA-256 checksum verified: $actual"
        return $true
    } else {
        Write-Err "SHA-256 mismatch!"
        Write-Err "  Expected: $ExpectedSha256"
        Write-Err "  Got:      $actual"
        return $false
    }
}

# ── File download ────────────────────────────────────────────────────────────
function Get-FileWithProgress {
    param(
        [string]$Url,
        [string]$Dest,
        [string]$Label,
        [string]$ExpectedSha256 = ""
    )

    Write-Step "Downloading $Label..."
    Write-Step "URL: $Url"

    try {
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        Invoke-WebRequest -Uri $Url -OutFile $Dest `
            -UserAgent "dri-installer/$VERSION (Windows; $arch)" `
            -ErrorAction Stop
        $sw.Stop()

        $size = [math]::Round((Get-Item $Dest).Length / 1KB, 1)
        Write-OK "$Label downloaded ($size KB, $($sw.Elapsed.TotalSeconds.ToString('F1'))s)"

        if (-not (Test-Checksum -FilePath $Dest -ExpectedSha256 $ExpectedSha256)) {
            Remove-Item $Dest -Force -ErrorAction SilentlyContinue
            return $false
        }

        return $true
    } catch [System.Net.WebException] {
        Write-Warn "$Label download failed: $($_.Exception.Message)"
        return $false
    } catch {
        Write-Warn "$Label download error: $($_.Exception.Message)"
        return $false
    }
}

# ── Register PATH ────────────────────────────────────────────────────────────
function Add-ToPath {
    param([string]$Dir)

    $isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator
    )

    $scope   = if ($isAdmin) { "Machine" } else { "User" }
    $rawPath = [Environment]::GetEnvironmentVariable("PATH", $scope)
    $current = if ($rawPath) { $rawPath } else { "" }

    if ($current -notlike "*$Dir*") {
        $newPath = if ($current) { "$current;$Dir" } else { $Dir }
        [Environment]::SetEnvironmentVariable("PATH", $newPath, $scope)
        Write-OK "Added to $scope PATH: $Dir"
    } else {
        Write-OK "Already in $scope PATH: $Dir"
    }

    if ($env:PATH -notlike "*$Dir*") {
        $env:PATH = "$env:PATH;$Dir"
    }

    # Broadcast WM_SETTINGCHANGE so Explorer and open terminals pick up the change
    try {
        $sig = @'
[DllImport("user32.dll", SetLastError=true, CharSet=CharSet.Auto)]
public static extern IntPtr SendMessageTimeout(
    IntPtr hWnd, uint Msg, UIntPtr wParam,
    string lParam, uint fuFlags, uint uTimeout, out UIntPtr lpdwResult);
'@
        $type   = Add-Type -MemberDefinition $sig -Name WinAPI -Namespace EnvBroadcast -PassThru
        $res    = [UIntPtr]::Zero
        $type::SendMessageTimeout(
            [IntPtr]0xFFFF, 0x001A, [UIntPtr]::Zero,
            "Environment", 2, 1000, [ref]$res
        ) | Out-Null
        Write-OK "PATH change broadcast to running processes"
    } catch {
        Write-Warn "Could not broadcast PATH change — open a new terminal to apply"
    }
}

# ── Register .dri file association ───────────────────────────────────────────
function Register-FileAssociation {
    param([string]$CompilerPath)

    try {
        New-Item -Path "HKCU:\Software\Classes\.dri"     -Force | Out-Null
        Set-ItemProperty "HKCU:\Software\Classes\.dri" "(Default)" "dri_source"

        New-Item -Path "HKCU:\Software\Classes\dri_source" -Force | Out-Null
        Set-ItemProperty "HKCU:\Software\Classes\dri_source" "(Default)" "dri Source File"

        $openKey = "HKCU:\Software\Classes\dri_source\shell\open\command"
        New-Item -Path $openKey -Force | Out-Null
        Set-ItemProperty $openKey "(Default)" "`"$CompilerPath`" `"%1`""

        Write-OK ".dri file association registered"
    } catch {
        Write-Warn ".dri file association failed (non-critical): $($_.Exception.Message)"
    }
}

# ── Install VSCode extension ──────────────────────────────────────────────────
function Install-VSCodeExtension {
    param([string]$VsixPath)

    $code = Get-Command "code" -ErrorAction SilentlyContinue
    if (-not $code) {
        Write-Warn "VSCode (code) not found — skipping extension install"
        Write-Warn "Manual install: code --install-extension `"$VsixPath`""
        return
    }

    Write-Step "Installing VSCode extension..."
    try {
        & code --install-extension "$VsixPath" --force 2>&1 | Out-Null
        Write-OK "VSCode extension installed"
    } catch {
        Write-Warn "VSCode extension install failed: $($_.Exception.Message)"
    }
}

# ── Local build fallback ──────────────────────────────────────────────────────
function Build-FromSource {
    param([string]$OutPath)

    Write-Step "Attempting local build from source..."

    # Resolve repo root: script is at <repo>/installer/window/install.ps1
    $scriptDir = Split-Path -Parent $MyInvocation.ScriptName
    $repoRoot  = Split-Path -Parent (Split-Path -Parent $scriptDir)

    $mainCpp = Join-Path $repoRoot "main.cpp"
    if (-not (Test-Path $mainCpp)) {
        Write-Warn "main.cpp not found at $repoRoot — skipping local build"
        return $false
    }

    $clang = Get-Command "clang++" -ErrorAction SilentlyContinue
    $gpp   = Get-Command "g++"     -ErrorAction SilentlyContinue

    if (-not $clang -and -not $gpp) {
        Write-Warn "No C++ compiler found (tried clang++ and g++)"
        return $false
    }

    $cxx = if ($clang) { "clang++" } else { "g++" }

    # All required source files
    $sources = @(
        (Join-Path $repoRoot "main.cpp")
        (Join-Path $repoRoot "compiler\src\lexer.cpp")
        (Join-Path $repoRoot "compiler\src\parser.cpp")
        (Join-Path $repoRoot "compiler\src\codegen.cpp")
        (Join-Path $repoRoot "compiler\src\compiler.cpp")
        (Join-Path $repoRoot "compiler\src\analyzer.cpp")
        (Join-Path $repoRoot "compiler\src\incremental.cpp")
    )

    $missingSrc = $sources | Where-Object { -not (Test-Path $_) }
    if ($missingSrc) {
        Write-Warn "Missing source files:"
        $missingSrc | ForEach-Object { Write-Host "    $_" -ForegroundColor Yellow }
        Write-Warn "Local build skipped due to missing files"
        return $false
    }

    $srcList   = $sources | ForEach-Object { "`"$_`"" }
    $buildArgs = "-std=c++20 -O2 -fopenmp -I`"$repoRoot`" $($srcList -join ' ') -o `"$OutPath`""

    Write-Step "Compiler: $cxx"
    Write-Step "Output  : $OutPath"

    try {
        $proc = Start-Process -FilePath $cxx -ArgumentList $buildArgs `
            -Wait -PassThru -NoNewWindow
        if ($proc.ExitCode -eq 0 -and (Test-Path $OutPath)) {
            Write-OK "Local build succeeded: $OutPath"
            return $true
        } else {
            Write-Warn "Build exited with code $($proc.ExitCode)"
        }
    } catch {
        Write-Warn "Build failed: $($_.Exception.Message)"
    }
    return $false
}

# ── Write installation info to registry ──────────────────────────────────────
function Write-Registry {
    param([string]$InstallPath)

    New-Item -Path $REG_PATH -Force | Out-Null
    Set-ItemProperty $REG_PATH "Version"      $VERSION
    Set-ItemProperty $REG_PATH "InstallDir"   $InstallDir
    Set-ItemProperty $REG_PATH "Compiler"     $InstallPath
    Set-ItemProperty $REG_PATH "InstalledAt"  (Get-Date -Format "yyyy-MM-dd HH:mm:ss")
    Set-ItemProperty $REG_PATH "Architecture" $arch.ToString()
    Write-OK "Registry entry written: $REG_PATH"
}

# ── Main install flow ─────────────────────────────────────────────────────────
function Invoke-Install {
    Show-Banner

    if ($Uninstall) {
        Invoke-Uninstall
        return
    }

    if ($Verify) {
        Invoke-Verify
        return
    }

    Write-Host "  Install location : $InstallDir" -ForegroundColor Gray
    Write-Host "  Architecture     : $arch"        -ForegroundColor Gray
    Write-Host "  Download binary  : $COMPILER_NAME" -ForegroundColor Gray
    Write-Host "  Silent mode      : $Silent"       -ForegroundColor Gray
    Write-Host ""

    if (-not $Silent) {
        $confirm = Read-Host "  Proceed with installation? [Y/n]"
        if ($confirm -and $confirm -notmatch '^[Yy]') {
            Write-Warn "Installation cancelled by user"
            return
        }
    }

    # 1. Create installation directory
    Write-Step "Creating installation directory..."
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    Write-OK "Directory ready: $InstallDir"

    $compilerPath = Join-Path $InstallDir $COMPILER_INSTALL_NAME
    $downloaded   = $false

    # 2. Download compiler binary (ARM64 or x64)
    $binaryUrl = "$RELEASE_URL/$COMPILER_NAME"
    $downloaded = Get-FileWithProgress -Url $binaryUrl -Dest $compilerPath -Label $COMPILER_NAME

    # Rename arm64 binary to dri.exe if needed
    if ($isArm64 -and $downloaded) {
        $armDest = Join-Path $InstallDir "dri-arm64.exe"
        if ((Test-Path $armDest) -and -not (Test-Path $compilerPath)) {
            Move-Item $armDest $compilerPath -Force
        }
    }

    # 3. Fall back to local build if download failed
    if (-not $downloaded -or -not (Test-Path $compilerPath)) {
        Write-Warn "Download failed — trying local build from source"
        $built = Build-FromSource -OutPath $compilerPath
        if (-not $built) {
            Write-Err "Installation failed: could not obtain dri.exe"
            Write-Host ""
            Write-Host "  Manual installation steps:" -ForegroundColor Yellow
            Write-Host "    1. Download dri.exe from: $RELEASE_URL/dri.exe" -ForegroundColor White
            Write-Host "    2. Copy dri.exe to: $InstallDir" -ForegroundColor White
            Write-Host "    3. Add $InstallDir to your PATH" -ForegroundColor White
            exit 1
        }
    }

    # 4. Register PATH
    Write-Step "Registering PATH..."
    Add-ToPath -Dir $InstallDir

    # 5. Register .dri file association
    Write-Step "Registering .dri file association..."
    Register-FileAssociation -CompilerPath $compilerPath

    # 6. Download and install VSCode extension
    if (-not $NoVSCode) {
        $vsixUrl  = "$RELEASE_URL/$VSIX_NAME"
        $vsixPath = Join-Path $InstallDir $VSIX_NAME

        $vsixOk = Get-FileWithProgress -Url $vsixUrl -Dest $vsixPath -Label $VSIX_NAME
        if ($vsixOk) {
            Install-VSCodeExtension -VsixPath $vsixPath
        }
    }

    # 7. Install build tools (LLVM / CMake) if missing
    Install-BuildTools

    # 8. Verify build tools
    Write-Host ""
    Write-Step "Verifying build environment..."
    Test-Dependency "clang++" "clang++" | Out-Null
    Test-Dependency "g++"     "g++"     | Out-Null
    Test-Dependency "cmake"   "cmake"   | Out-Null
    Test-Dependency "VSCode"  "code"    | Out-Null

    # 9. Write registry entry (Version, InstallDir, Compiler, InstalledAt, Architecture)
    Write-Registry -InstallPath $compilerPath

    # 10. Summary
    Write-Host ""
    Write-Host "  =======================================" -ForegroundColor DarkGreen
    Write-OK "dri compiler v$VERSION installed!"
    Write-Host "  =======================================" -ForegroundColor DarkGreen
    Write-Host ""
    Write-Host "  Usage examples:" -ForegroundColor Cyan
    Write-Host "    dri hello.dri --exe hello.exe" -ForegroundColor White
    Write-Host "    dri hello.dri --cpp hello.cpp"  -ForegroundColor White
    Write-Host "    dri hello.dri --check"          -ForegroundColor White
    Write-Host ""
    Write-Host "  Verify installation:" -ForegroundColor Cyan
    Write-Host "    powershell -File install.ps1 -Verify" -ForegroundColor White
    Write-Host ""
    Write-Host "  Uninstall:" -ForegroundColor Cyan
    Write-Host "    powershell -File install.ps1 -Uninstall" -ForegroundColor White
    Write-Host ""
    Write-Host "  Open a new terminal window for PATH changes to take effect." -ForegroundColor Yellow
    Write-Host ""

    # 11. Verify dri binary runs
    if (Test-Path $compilerPath) {
        Write-Step "Verifying installed binary..."
        try {
            $ver = & $compilerPath "--version" 2>&1
            Write-OK "Binary OK: $ver"
        } catch {
            Write-Warn "Binary did not respond — open a new terminal and run: dri --version"
        }
    }
}

# ── Entry point ───────────────────────────────────────────────────────────────
Invoke-Install
