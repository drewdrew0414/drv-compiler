#Requires -Version 5.1
<#
.SYNOPSIS
    dri language compiler Windows installer script
.DESCRIPTION
    Downloads dri.exe from drvpm-registry.cloud, registers PATH,
    installs VSCode extension, and registers .dri file association
.PARAMETER InstallDir
    Installation directory (default: %LOCALAPPDATA%\dri)
.PARAMETER NoVSCode
    Skip VSCode extension installation
.PARAMETER Uninstall
    Remove dri compiler and all associated settings
#>
param(
    [string]$InstallDir = "$env:LOCALAPPDATA\dri",
    [switch]$NoVSCode,
    [switch]$Uninstall
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ── Constants ───────────────────────────────────────────────────────────────
$VERSION       = "0.1.0"
$REGISTRY_URL  = "https://drvpm-registry.cloud"
$RELEASE_URL   = "$REGISTRY_URL/dist/v$VERSION"
$COMPILER_NAME = "dri.exe"
$VSIX_NAME     = "dri-lang-$VERSION.vsix"
$REG_PATH      = "HKCU:\Software\dri-lang"

# ── Console output helpers ──────────────────────────────────────────────────
function Write-Step  { param($msg) Write-Host "  ► $msg" -ForegroundColor Cyan }
function Write-OK    { param($msg) Write-Host "  ✓ $msg" -ForegroundColor Green }
function Write-Warn  { param($msg) Write-Host "  ⚠ $msg" -ForegroundColor Yellow }
function Write-Err   { param($msg) Write-Host "  ✗ $msg" -ForegroundColor Red }

function Show-Banner {
    Write-Host ""
    Write-Host "  ╔══════════════════════════════════════╗" -ForegroundColor DarkCyan
    Write-Host "  ║   dri Language Compiler v$VERSION      ║" -ForegroundColor DarkCyan
    Write-Host "  ║   HPC Systems Language               ║" -ForegroundColor DarkCyan
    Write-Host "  ╚══════════════════════════════════════╝" -ForegroundColor DarkCyan
    Write-Host ""
}

# ── Uninstall ───────────────────────────────────────────────────────────────
function Invoke-Uninstall {
    Write-Step "Removing dri compiler..."

    # Remove from PATH (both Machine and User scopes)
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

    # Remove installation directory
    if (Test-Path $InstallDir) {
        Remove-Item $InstallDir -Recurse -Force
        Write-OK "Installation directory removed: $InstallDir"
    }

    # Remove registry entry
    if (Test-Path $REG_PATH) {
        Remove-Item $REG_PATH -Recurse -Force
        Write-OK "Registry entry removed"
    }

    Write-Host ""
    Write-OK "dri uninstalled"
    return
}

# ── Install build tools (GCC / CMake) if missing ───────────────────────────
function Install-BuildTools {

    $needGcc   = -not (Get-Command "g++"     -ErrorAction SilentlyContinue) -and
                 -not (Get-Command "clang++" -ErrorAction SilentlyContinue)
    $needCmake = -not (Get-Command "cmake"   -ErrorAction SilentlyContinue)

    if (-not $needGcc -and -not $needCmake) {
        Write-OK "C++ compiler and CMake already present"
        return
    }

    Write-Host ""
    Write-Step "Missing build tools detected — attempting auto-install..."

    # ── Try winget ────────────────────────────────────────────────────────────
    $hasWinget = Get-Command "winget" -ErrorAction SilentlyContinue
    if ($hasWinget) {
        Write-Step "Using winget..."

        if ($needGcc) {
            Write-Step "Installing LLVM (clang++) via winget..."
            try {
                winget install --id LLVM.LLVM -e --silent --accept-package-agreements --accept-source-agreements
                # Refresh PATH so clang++ is visible immediately
                $llvmBin = "$env:ProgramFiles\LLVM\bin"
                if (Test-Path $llvmBin) { $env:PATH = "$env:PATH;$llvmBin" }
                if (Get-Command "clang++" -ErrorAction SilentlyContinue) {
                    Write-OK "clang++ installed"
                    $needGcc = $false
                } else {
                    Write-Warn "clang++ install may require a new terminal to take effect"
                    $needGcc = $false
                }
            } catch {
                Write-Warn "LLVM install failed: $($_.Exception.Message)"
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
                    Write-Warn "cmake install may require a new terminal to take effect"
                    $needCmake = $false
                }
            } catch {
                Write-Warn "CMake install failed: $($_.Exception.Message)"
            }
        }
    }

    # ── Try Chocolatey ────────────────────────────────────────────────────────
    if ($needGcc -or $needCmake) {
        $hasChoco = Get-Command "choco" -ErrorAction SilentlyContinue

        if (-not $hasChoco) {
            # Install Chocolatey itself
            Write-Step "winget unavailable — installing Chocolatey..."
            try {
                Set-ExecutionPolicy Bypass -Scope Process -Force
                [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
                Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
                $env:PATH = "$env:PATH;$env:ALLUSERSPROFILE\chocolatey\bin"
                $hasChoco = Get-Command "choco" -ErrorAction SilentlyContinue
                if ($hasChoco) { Write-OK "Chocolatey installed" }
            } catch {
                Write-Warn "Chocolatey install failed: $($_.Exception.Message)"
            }
        }

        if ($hasChoco -or (Get-Command "choco" -ErrorAction SilentlyContinue)) {
            if ($needGcc) {
                Write-Step "Installing MinGW (g++) via Chocolatey..."
                try {
                    choco install mingw -y --no-progress
                    $mingwBin = "$env:SystemDrive\tools\mingw64\bin"
                    if (Test-Path $mingwBin) { $env:PATH = "$env:PATH;$mingwBin" }
                    if (Get-Command "g++" -ErrorAction SilentlyContinue) {
                        Write-OK "g++ installed"
                        $needGcc = $false
                    } else {
                        Write-Warn "g++ installed — open a new terminal to use it"
                        $needGcc = $false
                    }
                } catch {
                    Write-Warn "MinGW install failed: $($_.Exception.Message)"
                }
            }

            if ($needCmake) {
                Write-Step "Installing CMake via Chocolatey..."
                try {
                    choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' -y --no-progress
                    if (Get-Command "cmake" -ErrorAction SilentlyContinue) {
                        Write-OK "cmake installed"
                        $needCmake = $false
                    } else {
                        Write-Warn "cmake installed — open a new terminal to use it"
                        $needCmake = $false
                    }
                } catch {
                    Write-Warn "CMake install failed: $($_.Exception.Message)"
                }
            }
        }
    }

    # ── Still missing — show manual instructions ──────────────────────────────
    if ($needGcc) {
        Write-Warn "Could not auto-install C++ compiler. Install manually:"
        Write-Host "    LLVM  : https://github.com/llvm/llvm-project/releases (LLVM-*-win64.exe)" -ForegroundColor Yellow
        Write-Host "    MinGW : https://github.com/niXman/mingw-builds-binaries/releases"         -ForegroundColor Yellow
    }
    if ($needCmake) {
        Write-Warn "Could not auto-install CMake. Install manually:"
        Write-Host "    CMake : https://cmake.org/download/ (cmake-*-windows-x86_64.msi)"         -ForegroundColor Yellow
    }
}

# ── Dependency check ────────────────────────────────────────────────────────
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

# ── File download ───────────────────────────────────────────────────────────
function Get-FileWithProgress {
    param([string]$Url, [string]$Dest, [string]$Label)

    Write-Step "Downloading $Label..."
    Write-Step "URL: $Url"

    try {
        $client = New-Object System.Net.WebClient
        $client.Headers.Add("User-Agent", "dri-installer/$VERSION")

        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        $client.DownloadFile($Url, $Dest)
        $sw.Stop()

        $size = [math]::Round((Get-Item $Dest).Length / 1KB, 1)
        Write-OK "$Label downloaded ($size KB, $($sw.Elapsed.TotalSeconds.ToString('F1'))s)"
        return $true
    } catch [System.Net.WebException] {
        Write-Warn "$Label download failed: $($_.Exception.Message)"
        return $false
    }
}

# ── Register PATH ───────────────────────────────────────────────────────────
function Add-ToPath {
    param([string]$Dir)

    $isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator
    )

    if ($isAdmin) {
        # Machine-level PATH — applies to all users
        $scope   = "Machine"
        $rawPath = [Environment]::GetEnvironmentVariable("PATH", "Machine")
    } else {
        # User-level PATH
        $scope   = "User"
        $rawPath = [Environment]::GetEnvironmentVariable("PATH", "User")
    }

    $current = if ($rawPath) { $rawPath } else { "" }

    if ($current -notlike "*$Dir*") {
        $newPath = if ($current) { "$current;$Dir" } else { $Dir }
        [Environment]::SetEnvironmentVariable("PATH", $newPath, $scope)
        Write-OK "Added to $scope PATH: $Dir"
    } else {
        Write-OK "Already in $scope PATH: $Dir"
    }

    # Apply to current session immediately
    if ($env:PATH -notlike "*$Dir*") {
        $env:PATH = "$env:PATH;$Dir"
    }

    # Broadcast WM_SETTINGCHANGE so Explorer / open terminals pick up the change
    try {
        $sig = @'
[DllImport("user32.dll", SetLastError=true, CharSet=CharSet.Auto)]
public static extern IntPtr SendMessageTimeout(
    IntPtr hWnd, uint Msg, UIntPtr wParam,
    string lParam, uint fuFlags, uint uTimeout, out UIntPtr lpdwResult);
'@
        $type   = Add-Type -MemberDefinition $sig -Name WinAPI -Namespace Env -PassThru
        $result = [UIntPtr]::Zero
        $type::SendMessageTimeout(
            [IntPtr]0xFFFF, 0x001A, [UIntPtr]::Zero,
            "Environment", 2, 1000, [ref]$result
        ) | Out-Null
        Write-OK "PATH change broadcast to running processes"
    } catch {
        Write-Warn "Could not broadcast PATH change — open a new terminal to apply"
    }
}

# ── Register .dri file association (Explorer icon / open-with) ──────────────
function Register-FileAssociation {
    param([string]$CompilerPath)

    try {
        # .dri → dri_source ProgID
        New-Item -Path "HKCU:\Software\Classes\.dri"     -Force | Out-Null
        Set-ItemProperty "HKCU:\Software\Classes\.dri" "(Default)" "dri_source"

        # ProgID description
        New-Item -Path "HKCU:\Software\Classes\dri_source" -Force | Out-Null
        Set-ItemProperty "HKCU:\Software\Classes\dri_source" "(Default)" "dri Source File"

        # Open command
        $openKey = "HKCU:\Software\Classes\dri_source\shell\open\command"
        New-Item -Path $openKey -Force | Out-Null
        Set-ItemProperty $openKey "(Default)" "`"$CompilerPath`" `"%1`""

        Write-OK ".dri file association registered"
    } catch {
        Write-Warn ".dri file association failed (non-critical): $($_.Exception.Message)"
    }
}

# ── Install VSCode extension ────────────────────────────────────────────────
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

# ── Local build fallback (when download fails) ──────────────────────────────
function Build-FromSource {
    param([string]$OutPath)

    Write-Step "Attempting local build from source..."

    $scriptDir  = Split-Path -Parent $MyInvocation.ScriptName
    $repoRoot   = Split-Path -Parent $scriptDir
    $mainCpp    = Join-Path $repoRoot "main.cpp"

    if (-not (Test-Path $mainCpp)) {
        Write-Warn "main.cpp not found — skipping local build"
        return $false
    }

    $clang = Get-Command "clang++" -ErrorAction SilentlyContinue
    $gpp   = Get-Command "g++"     -ErrorAction SilentlyContinue

    if (-not $clang -and -not $gpp) {
        Write-Warn "No C++ compiler found (clang++ / g++)"
        return $false
    }

    $cxx = if ($clang) { "clang++" } else { "g++" }
    $src = @(
        "$repoRoot\main.cpp"
        "$repoRoot\compiler\src\lexer.cpp"
        "$repoRoot\compiler\src\parser.cpp"
        "$repoRoot\compiler\src\codegen.cpp"
        "$repoRoot\compiler\src\compiler.cpp"
    )

    $buildCmd = "$cxx -std=c++20 -O2 -I`"$repoRoot`" $($src -join ' ') -o `"$OutPath`""
    Write-Step "Build: $buildCmd"

    try {
        Invoke-Expression $buildCmd
        if (Test-Path $OutPath) {
            Write-OK "Local build succeeded: $OutPath"
            return $true
        }
    } catch {
        Write-Warn "Build failed: $($_.Exception.Message)"
    }
    return $false
}

# ── Write installation info to registry ────────────────────────────────────
function Write-Registry {
    param([string]$InstallPath)

    New-Item -Path $REG_PATH -Force | Out-Null
    Set-ItemProperty $REG_PATH "Version"     $VERSION
    Set-ItemProperty $REG_PATH "InstallDir"  $InstallDir
    Set-ItemProperty $REG_PATH "Compiler"    $InstallPath
    Set-ItemProperty $REG_PATH "InstalledAt" (Get-Date -Format "yyyy-MM-dd HH:mm:ss")
    Write-OK "Registry entry written"
}

# ── Main install flow ───────────────────────────────────────────────────────
function Invoke-Install {
    Show-Banner

    if ($Uninstall) {
        Invoke-Uninstall
        return
    }

    Write-Host "  Install location: $InstallDir" -ForegroundColor Gray
    Write-Host ""

    # 1. Create installation directory
    Write-Step "Creating installation directory..."
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    Write-OK "Directory ready: $InstallDir"

    $compilerPath = Join-Path $InstallDir $COMPILER_NAME
    $downloaded   = $false

    # 2. Download compiler from registry
    $releaseUrl = "$RELEASE_URL/$COMPILER_NAME"

    # [수정] TLS 1.2 / 1.3 활성화 코드를 다운로드 직전에 강제 적용
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072 -bor 12288

    # [수정] 기존 Get-FileWithProgress 대신 PowerShell 내장 가속 명령어 사용
    Write-Step "Downloading $COMPILER_NAME..."
    Write-Step "URL: $releaseUrl"
    try {
        $sw = [System.Diagnostics.Stopwatch]::StartNew()

        # 웹 브라우저처럼 인식하도록 UserAgent를 추가하여 다운로드 차단 우회
        Invoke-WebRequest -Uri $releaseUrl -OutFile $compilerPath -UserAgent "Mozilla/5.0 (Windows NT 10.0; Win64; x64)" -ErrorAction Stop

        $sw.Stop()
        $size = [math]::Round((Get-Item $compilerPath).Length / 1KB, 1)
        Write-OK "dri.exe downloaded ($size KB, $($sw.Elapsed.TotalSeconds.ToString('F1'))s)"
        $downloaded = $true
    } catch {
        Write-Warn "dri.exe download failed: $($_.Exception.Message)"
        $downloaded = $false
    }

    # 3. Fall back to local build if download failed
    if (-not $downloaded -or -not (Test-Path $compilerPath)) {
        Write-Warn "Registry download failed — trying local build"
        $built = Build-FromSource -OutPath $compilerPath
        if (-not $built) {
            Write-Err "Installation failed: could not obtain compiler binary"
            Write-Host ""
            Write-Host "  Manual installation:" -ForegroundColor Yellow
            Write-Host "  1. Download drv.exe from https://drvpm-registry.cloud/dist/v$VERSION/drv.exe"
            Write-Host "  2. Copy to $InstallDir"
            Write-Host "  3. Add $InstallDir to PATH"
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

        Write-Step "Downloading VSCode extension..."
        try {
            Invoke-WebRequest -Uri $vsixUrl -OutFile $vsixPath -UserAgent "Mozilla/5.0 (Windows NT 10.0; Win64; x64)" -ErrorAction Stop
            $vsixOk = $true
        } catch {
            Write-Warn "VSCode extension download failed: $($_.Exception.Message)"
            $vsixOk = $false
        }

        if ($vsixOk) {
            Install-VSCodeExtension -VsixPath $vsixPath
        }
    }

    # 7. Install GCC / CMake if missing
    Install-BuildTools

    # 8. Verify build tools
    Write-Host ""
    Write-Step "Verifying build tools..."
    $hasClang = Test-Dependency "clang++" "clang++"
    $hasGpp   = Test-Dependency "g++"     "g++"
    Test-Dependency "cmake"  "cmake"  | Out-Null
    Test-Dependency "VSCode" "code"   | Out-Null

    # 9. Write registry entry
    Write-Registry -InstallPath $compilerPath

    # 10. Summary
    Write-Host ""
    Write-Host "  ═══════════════════════════════════════" -ForegroundColor DarkGreen
    Write-OK "dri compiler v$VERSION installed!"
    Write-Host "  ═══════════════════════════════════════" -ForegroundColor DarkGreen
    Write-Host ""
    Write-Host "  Usage:" -ForegroundColor Cyan
    Write-Host "    dri hello.dri --exe hello.exe" -ForegroundColor White
    Write-Host "    dri hello.dri --cpp hello.cpp" -ForegroundColor White
    Write-Host "    dri hello.dri --check" -ForegroundColor White
    Write-Host ""
    Write-Host "  Uninstall:" -ForegroundColor Cyan
    Write-Host "    iex `"$($MyInvocation.ScriptName) -Uninstall`"" -ForegroundColor White
    Write-Host ""

    # 10. Verify installation
    # 11. Verify dri compiler
    if (Test-Path $compilerPath) {
        Write-Step "Verifying installation..."
        try {
            $ver = & $compilerPath "--version" 2>&1
            Write-OK "Compiler OK: $ver"
        } catch {
            Write-Warn "Compiler did not run — open a new terminal and retry"
        }
    }
}

# ── Entry point ──────────────────────────────────────────────────────────────
Invoke-Install
