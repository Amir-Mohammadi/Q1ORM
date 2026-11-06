@echo off
setlocal EnableExtensions EnableDelayedExpansion

echo ============================================
echo           Q1ORM Release Builder
echo ============================================

rem ---- Project paths (auto-derived from this script's location) ----
set "PROJECT=%~dp0"
if "%PROJECT:~-1%"=="\" set "PROJECT=%PROJECT:~0,-1%"
set "BUILD=%PROJECT%\build"
if defined Q1ORM_BUILD_DIR set "BUILD=%Q1ORM_BUILD_DIR%"
set "RELEASE=%PROJECT%\Releases\Release-0.1"
if defined Q1ORM_RELEASE_DIR set "RELEASE=%Q1ORM_RELEASE_DIR%"
if not defined Q1ORM_RELEASE_SQL_DRIVERS set "Q1ORM_RELEASE_SQL_DRIVERS=QMYSQL;QSQLITE"

rem ---- Auto-detect Qt (QTDIR env -> newest C:\Qt\<ver>\msvc2022_64 -> qmake on PATH) ----
set "QT="
if defined QTDIR if exist "%QTDIR%\bin\qmake.exe" set "QT=%QTDIR%"
if not defined QT if exist "C:\Qt" (
    for /f "delims=" %%d in ('dir /b /ad /o-n "C:\Qt" 2^>nul') do (
        if not defined QT if exist "C:\Qt\%%d\msvc2022_64" set "QT=C:\Qt\%%d\msvc2022_64"
    )
)
if not defined QT (
    where qmake >nul 2>nul
    if not errorlevel 1 for /f "delims=" %%p in ('qmake -query QT_INSTALL_PREFIX') do set "QT=%%p"
)
if not defined QT echo WARNING: Qt not found. Set QTDIR or run from a Qt-enabled prompt.

rem ---- Auto-detect MSVC (validated: vcvars must expose MSVC toolset headers) ----
set "VCVARS="
set "VS_CANDIDATES="
if exist "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" (
    for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -all -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" set "VS_CANDIDATES=!VS_CANDIDATES! "%%i\VC\Auxiliary\Build\vcvars64.bat""
    )
)
if not defined VS_CANDIDATES set "VS_CANDIDATES="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat""
for %%C in (%VS_CANDIDATES%) do (
    if not defined VCVARS (
        call :check_vcvars %%C
        if not errorlevel 1 set "VCVARS=%%~C"
    )
)
goto :vcvars_done

:check_vcvars
call "%~1" >nul 2>nul
echo !INCLUDE! | findstr /C:"VC\Tools\MSVC" >nul
exit /b %errorlevel%

:vcvars_done
if not defined VCVARS echo WARNING: No usable MSVC toolchain found - Qt Creator trees will be skipped.

rem ---- Auto-detect jom (for Qt Creator build trees) ----
set "JOM_DIR="
if exist "C:\Qt\Tools\QtCreator\bin\jom\jom.exe" set "JOM_DIR=C:\Qt\Tools\QtCreator\bin\jom"

echo.
echo Project : %PROJECT%
echo Qt      : %QT%
echo Build   : %BUILD%
echo.

echo.
echo [1/6] Configuring project...
if defined QT (
    cmake -S "%PROJECT%" -B "%BUILD%" -DQ1ORM_BUILD_TOOLS=ON -DCMAKE_PREFIX_PATH="%QT%" -DQ1ORM_RELEASE_DIR="%RELEASE%" -DQ1ORM_RELEASE_SQL_DRIVERS="%Q1ORM_RELEASE_SQL_DRIVERS%"
) else (
    cmake -S "%PROJECT%" -B "%BUILD%" -DQ1ORM_BUILD_TOOLS=ON -DQ1ORM_RELEASE_DIR="%RELEASE%" -DQ1ORM_RELEASE_SQL_DRIVERS="%Q1ORM_RELEASE_SQL_DRIVERS%"
)

if errorlevel 1 (
    echo CONFIGURE FAILED!
    if not defined Q1ORM_NO_PAUSE pause
    exit /b 1
)

echo.
echo [2/6] Building library...
cmake --build "%BUILD%" --target Src --config Release

if errorlevel 1 (
    echo LIBRARY BUILD FAILED!
    if not defined Q1ORM_NO_PAUSE pause
    exit /b 1
)

echo.
echo [3/6] Building tools...
cmake --build "%BUILD%" --target ExampleProjectModifier --config Release

if errorlevel 1 (
    echo EXAMPLE PROJECT MODIFIER BUILD FAILED!
    if not defined Q1ORM_NO_PAUSE pause
    exit /b 1
)

echo.
echo [4/6] Building examples (all registered example projects)...
cmake --build "%BUILD%" --config Release

if errorlevel 1 (
    echo EXAMPLES BUILD FAILED!
    if not defined Q1ORM_NO_PAUSE pause
    exit /b 1
)

echo.
echo [5/6] Copying fresh Q1ORM.dll next to released example executables...
for /d %%E in ("%BUILD%\Examples\*") do (
    if exist "%%E\Release" (
        copy /y "%BUILD%\src\Release\Q1ORM.dll" "%%E\Release\Q1ORM.dll" >nul
    )
)

echo.
echo [6/6] Syncing Qt Creator build trees (Debug and Release)...
set "QT_DEBUG_BUILD="
set "QT_RELEASE_BUILD="
for /d %%D in ("%BUILD%\Desktop_Qt_*_Debug") do set "QT_DEBUG_BUILD=%%D"
for /d %%D in ("%BUILD%\Desktop_Qt_*_Release") do set "QT_RELEASE_BUILD=%%D"

if defined JOM_DIR set "PATH=%JOM_DIR%;%PATH%"
if defined VCVARS call "%VCVARS%" >nul

if defined QT_DEBUG_BUILD (
    if exist "%QT_DEBUG_BUILD%\CMakeCache.txt" (
        echo Building all in Qt Creator Debug tree...
        cmake --build "%QT_DEBUG_BUILD%"
        if errorlevel 1 (
            echo QT DEBUG BUILD FAILED!
            if not defined Q1ORM_NO_PAUSE pause
            exit /b 1
        )
    ) else (
        echo INFO: No Qt Creator Debug tree configured yet - skipping.
    )
) else (
    echo INFO: No Qt Creator Debug tree found - skipping.
)

if defined QT_RELEASE_BUILD (
    if exist "%QT_RELEASE_BUILD%\CMakeCache.txt" (
        echo Building all in Qt Creator Release tree...
        cmake --build "%QT_RELEASE_BUILD%"
        if errorlevel 1 (
            echo QT RELEASE BUILD FAILED!
            if not defined Q1ORM_NO_PAUSE pause
            exit /b 1
        )
    ) else (
        echo INFO: No Qt Creator Release tree configured yet - skipping.
    )
) else (
    echo INFO: No Qt Creator Release tree found - skipping.
)

echo.
echo Installing library and examples...
cmake --install "%BUILD%" --config Release --prefix "%RELEASE%"

if errorlevel 1 (
    echo INSTALL FAILED!
    if not defined Q1ORM_NO_PAUSE pause
    exit /b 1
)

echo.
echo Verifying release...

if not exist "%RELEASE%" (
    echo RELEASE DIRECTORY NOT FOUND!
    if not defined Q1ORM_NO_PAUSE pause
    exit /b 1
)

echo.
echo Packaging Qt SQL plugins and native client dependencies into the release...
cmake --build "%BUILD%" --target Q1ORMReleaseRuntime --config Release
if errorlevel 1 (
    echo RELEASE RUNTIME PACKAGING FAILED! See the missing dependency or loader error above.
    if not defined Q1ORM_NO_PAUSE pause
    exit /b 1
)

echo.
echo ============================================
echo     RELEASE CREATED SUCCESSFULLY
echo ============================================
echo.
echo Release folder:
echo %RELEASE%
echo.
echo Contents of bin:
if exist "%RELEASE%\bin" (
    dir /b "%RELEASE%\bin"
) else (
    echo WARNING: bin directory not found.
)

echo.
if not defined Q1ORM_NO_PAUSE pause
