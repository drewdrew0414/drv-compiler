@echo off
REM ============================================================================
REM  build-installer.bat — builds dri-install.exe from launcher.cs
REM  Tries: .NET 8 SDK (dotnet build) -> .NET 4.x csc.exe -> Roslyn csc
REM  Outputs: dri-install.exe (x64), optionally dri-install-arm64.exe
REM  CI-friendly: no unconditional pause at end
REM ============================================================================
setlocal EnableDelayedExpansion

set "VERSION=0.1.0"
set "SCRIPT=%~0"
set "INVOKED_AS=%0"
set "SRC=launcher.cs"
set "OUT_X64=dri-install.exe"
set "OUT_ARM64=dri-install-arm64.exe"
set "BUILD_ARM64=0"
set "USE_DOTNET=0"
set "CSC="

REM ── Parse optional arguments ─────────────────────────────────────────────
:parse_args
if "%~1"=="" goto :after_args
if /i "%~1"=="/arm64"   set "BUILD_ARM64=1" & shift & goto :parse_args
if /i "%~1"=="--arm64"  set "BUILD_ARM64=1" & shift & goto :parse_args
if /i "%~1"=="/x64"     set "BUILD_ARM64=0" & shift & goto :parse_args
if /i "%~1"=="--x64"    set "BUILD_ARM64=0" & shift & goto :parse_args
shift
goto :parse_args
:after_args

echo.
echo  ============================================================
echo   dri Installer Build Script  v%VERSION%
echo  ============================================================
echo.

REM ── Check that source file exists ────────────────────────────────────────
if not exist "%SRC%" (
    echo  [ERROR] %SRC% not found in current directory.
    echo          Run this script from the installer\window\ folder.
    goto :fail
)

REM ── Try .NET 8+ SDK: dotnet build ────────────────────────────────────────
echo  [1/3] Checking for .NET 8 SDK (dotnet)...
where dotnet >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    for /f "tokens=*" %%v in ('dotnet --version 2^>nul') do set "DOTNET_VER=%%v"
    echo        Found dotnet !DOTNET_VER!

    REM Check if version starts with 8 or higher
    set "DOTNET_MAJOR=!DOTNET_VER:~0,1!"
    if !DOTNET_MAJOR! GEQ 8 (
        echo        Using dotnet build...
        set "USE_DOTNET=1"
        goto :build_dotnet
    ) else (
        echo        dotnet !DOTNET_VER! found but need 8+; trying csc fallback
    )
) else (
    echo        dotnet not found; trying csc fallback
)

REM ── Try .NET Framework csc.exe (Framework64 first, then Framework) ────────
echo  [2/3] Searching for .NET Framework csc.exe...

for /d %%v in ("%WINDIR%\Microsoft.NET\Framework64\v4*") do set "CSC=%%v\csc.exe"
if defined CSC (
    if not exist "!CSC!" set "CSC="
)

if not defined CSC (
    for /d %%v in ("%WINDIR%\Microsoft.NET\Framework\v4*") do set "CSC=%%v\csc.exe"
    if defined CSC (
        if not exist "!CSC!" set "CSC="
    )
)

if defined CSC (
    echo        Found csc: !CSC!
    goto :build_csc
)

REM ── Roslyn csc from Visual Studio or PATH ────────────────────────────────
echo  [3/3] Searching for Roslyn csc.exe...

for /f "delims=" %%p in ('where csc 2^>nul') do (
    set "CSC=%%p"
    goto :found_roslyn
)

REM Also search common VS install paths
for /d %%v in ("%ProgramFiles%\Microsoft Visual Studio\*\*\MSBuild\Current\Bin\Roslyn") do (
    if exist "%%v\csc.exe" (
        set "CSC=%%v\csc.exe"
        goto :found_roslyn
    )
)
for /d %%v in ("%ProgramFiles(x86)%\Microsoft Visual Studio\*\*\MSBuild\Current\Bin\Roslyn") do (
    if exist "%%v\csc.exe" (
        set "CSC=%%v\csc.exe"
        goto :found_roslyn
    )
)

goto :no_compiler

:found_roslyn
echo        Found Roslyn csc: !CSC!
goto :build_csc

:no_compiler
echo.
echo  [ERROR] No suitable C# compiler found.
echo.
echo  To fix, install one of:
echo    Option A: .NET 8 SDK   https://dotnet.microsoft.com/download
echo    Option B: .NET Framework 4.7.2 Developer Pack
echo              https://dotnet.microsoft.com/download/dotnet-framework/net472
echo    Option C: Visual Studio 2022 (Community is free)
echo              https://visualstudio.microsoft.com/
echo.
echo  Alternatively, run install.ps1 directly:
echo    powershell -ExecutionPolicy Bypass -File install.ps1
goto :fail

REM ============================================================================
:build_dotnet
REM  .NET 8 SDK path — create a minimal csproj if needed, then build
REM ============================================================================
echo.
echo  Building with dotnet (x64)...

REM Write a minimal csproj for the WinForms launcher
set "CSPROJ=dri-install-build.csproj"
(
echo ^<Project Sdk="Microsoft.NET.Sdk"^>
echo   ^<PropertyGroup^>
echo     ^<OutputType^>WinExe^</OutputType^>
echo     ^<TargetFramework^>net8.0-windows^</TargetFramework^>
echo     ^<UseWindowsForms^>true^</UseWindowsForms^>
echo     ^<Nullable^>disable^</Nullable^>
echo     ^<ImplicitUsings^>disable^</ImplicitUsings^>
echo     ^<Optimize^>true^</Optimize^>
echo     ^<AssemblyVersion^>%VERSION%.0^</AssemblyVersion^>
echo     ^<FileVersion^>%VERSION%.0^</FileVersion^>
echo     ^<RuntimeIdentifier^>win-x64^</RuntimeIdentifier^>
echo     ^<SelfContained^>false^</SelfContained^>
echo     ^<AssemblyName^>dri-install^</AssemblyName^>
echo   ^</PropertyGroup^>
echo   ^<ItemGroup^>
echo     ^<Compile Include="launcher.cs" /^>
echo   ^</ItemGroup^>
echo ^</Project^>
) > "%CSPROJ%"

dotnet build "%CSPROJ%" -c Release -o . --nologo
if %ERRORLEVEL% NEQ 0 (
    echo  [WARN] dotnet build failed; falling back to csc search...
    del /f /q "%CSPROJ%" >nul 2>&1
    goto :no_compiler
)

del /f /q "%CSPROJ%" >nul 2>&1

REM Locate output
if exist "dri-install.exe" (
    ren "dri-install.exe" "%OUT_X64%" >nul 2>&1
)

goto :post_build_x64

REM ============================================================================
:build_csc
REM  Classic csc.exe path
REM ============================================================================
echo.
echo  Building x64 with: !CSC!
echo  Output: %OUT_X64%

"!CSC!" /nologo /target:winexe /optimize+ /platform:x64 ^
    /r:System.dll ^
    /r:System.Windows.Forms.dll ^
    /r:System.Drawing.dll ^
    /r:System.Net.dll ^
    /r:System.ComponentModel.dll ^
    /out:"%OUT_X64%" ^
    "%SRC%"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo  [ERROR] x64 build failed.
    goto :fail
)

:post_build_x64
if not exist "%OUT_X64%" (
    echo  [ERROR] Output file not found: %OUT_X64%
    goto :fail
)

REM Show file size
for %%F in ("%OUT_X64%") do (
    set "SIZE_BYTES=%%~zF"
    set /a "SIZE_KB=!SIZE_BYTES! / 1024"
    echo.
    echo  Build complete: %OUT_X64%  ^(!SIZE_KB! KB^)
)

REM ── Optional ARM64 build ──────────────────────────────────────────────────
if "%BUILD_ARM64%"=="0" goto :summary

if "%USE_DOTNET%"=="1" (
    echo.
    echo  Building ARM64 with dotnet...

    set "CSPROJ2=dri-install-arm64-build.csproj"
    (
    echo ^<Project Sdk="Microsoft.NET.Sdk"^>
    echo   ^<PropertyGroup^>
    echo     ^<OutputType^>WinExe^</OutputType^>
    echo     ^<TargetFramework^>net8.0-windows^</TargetFramework^>
    echo     ^<UseWindowsForms^>true^</UseWindowsForms^>
    echo     ^<Nullable^>disable^</Nullable^>
    echo     ^<ImplicitUsings^>disable^</ImplicitUsings^>
    echo     ^<Optimize^>true^</Optimize^>
    echo     ^<RuntimeIdentifier^>win-arm64^</RuntimeIdentifier^>
    echo     ^<SelfContained^>false^</SelfContained^>
    echo     ^<AssemblyName^>dri-install-arm64^</AssemblyName^>
    echo   ^</PropertyGroup^>
    echo   ^<ItemGroup^>
    echo     ^<Compile Include="launcher.cs" /^>
    echo   ^</ItemGroup^>
    echo ^</Project^>
    ) > "!CSPROJ2!"

    dotnet build "!CSPROJ2!" -c Release -o . --nologo
    if %ERRORLEVEL% NEQ 0 (
        echo  [WARN] ARM64 dotnet build failed
    ) else (
        if exist "dri-install-arm64.exe" (
            echo  ARM64 build complete: %OUT_ARM64%
        )
    )
    del /f /q "!CSPROJ2!" >nul 2>&1

) else (
    echo.
    echo  Building ARM64 with: !CSC!
    echo  Output: %OUT_ARM64%

    "!CSC!" /nologo /target:winexe /optimize+ /platform:arm64 ^
        /r:System.dll ^
        /r:System.Windows.Forms.dll ^
        /r:System.Drawing.dll ^
        /r:System.Net.dll ^
        /r:System.ComponentModel.dll ^
        /out:"%OUT_ARM64%" ^
        "%SRC%"

    if %ERRORLEVEL% NEQ 0 (
        echo  [WARN] ARM64 build failed ^(csc may not support /platform:arm64 on this system^)
    ) else (
        for %%F in ("%OUT_ARM64%") do (
            set "ARM64_BYTES=%%~zF"
            set /a "ARM64_KB=!ARM64_BYTES! / 1024"
            echo  ARM64 build complete: %OUT_ARM64%  ^(!ARM64_KB! KB^)
        )
    )
)

:summary
echo.
echo  ============================================================
echo   Build Summary
echo  ============================================================
if exist "%OUT_X64%" (
    for %%F in ("%OUT_X64%") do (
        set /a "SZ=%%~zF / 1024"
        echo   x64  : %OUT_X64%  ^(!SZ! KB^)
    )
) else (
    echo   x64  : NOT BUILT
)
if exist "%OUT_ARM64%" (
    for %%F in ("%OUT_ARM64%") do (
        set /a "SZ=%%~zF / 1024"
        echo   ARM64: %OUT_ARM64%  ^(!SZ! KB^)
    )
)
echo.
echo   Run GUI installer : %OUT_X64%
echo   Run PS1 directly  : powershell -ExecutionPolicy Bypass -File install.ps1
echo   Sign the binary   : powershell -File sign.ps1
echo  ============================================================
echo.

REM ── Conditional pause: only when run interactively (double-clicked) ────────
REM  When invoked via cmd /c or CI, %0 differs from %~0
if "%INVOKED_AS%"=="%SCRIPT%" pause
goto :end

:fail
echo.
echo  Build FAILED.
echo.
if "%INVOKED_AS%"=="%SCRIPT%" pause
exit /b 1

:end
endlocal
exit /b 0
