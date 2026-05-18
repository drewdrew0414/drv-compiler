@echo off
REM Build script for dri-install.exe (WinForms GUI installer)
REM Run: build-installer.bat

echo Building dri installer...

REM Locate .NET Framework csc.exe
set CSC=
for /d %%v in ("%WINDIR%\Microsoft.NET\Framework64\v4*") do set CSC=%%v\csc.exe
if not exist "%CSC%" (
    for /d %%v in ("%WINDIR%\Microsoft.NET\Framework\v4*") do set CSC=%%v\csc.exe
)

REM Fallback: Roslyn csc from Visual Studio
if not exist "%CSC%" (
    for /f "delims=" %%p in ('where csc 2^>nul') do (
        set CSC=%%p
        goto :found_csc
    )
)

:found_csc
if not exist "%CSC%" (
    echo Error: csc.exe not found. Install .NET Framework 4.x or Visual Studio.
    echo Alternative: run install.ps1 directly:
    echo   powershell -ExecutionPolicy Bypass -File install.ps1
    pause
    exit /b 1
)

echo CSC: %CSC%

"%CSC%" /nologo /target:winexe /optimize+ /r:System.Windows.Forms.dll /r:System.Drawing.dll /out:dri-install.exe launcher.cs

if %ERRORLEVEL% == 0 (
    echo.
    echo Build complete: dri-install.exe
    echo.
    echo Usage:
    echo   dri-install.exe              ^(GUI installer^)
    echo   install.ps1 -NoVSCode        ^(skip VSCode extension^)
    echo   install.ps1 -Uninstall       ^(remove dri^)
) else (
    echo Build failed
)
pause
