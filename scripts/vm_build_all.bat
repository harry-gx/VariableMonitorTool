@echo off
setlocal EnableExtensions

set "ROOT=%~dp0.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"

set "QT_BIN=C:\Qt\Qt5.9.0\5.9\mingw53_32\bin"
set "MINGW_BIN=C:\Qt\Qt5.9.0\Tools\mingw530_32\bin"
set "QMAKE=%QT_BIN%\qmake.exe"
set "WINDEPLOYQT=%QT_BIN%\windeployqt.exe"
set "MINGW_MAKE=%MINGW_BIN%\mingw32-make.exe"
set "BUILD_ROOT=%ROOT%\build"
set "QT_BUILD=%BUILD_ROOT%\qt"
set "QT_BIN_OUT=%QT_BUILD%\bin"
set "PACKAGE_ROOT=%BUILD_ROOT%\package"
set "PACKAGE_DIR=%PACKAGE_ROOT%\VariableMonitorTool"
set "PRO_FILE=%ROOT%\vm_ui\vm_VariableMonitorTool.pro"
set "EXE_FILE=%QT_BIN_OUT%\VariableMonitorTool.exe"

set "PATH=%QT_BIN%;%MINGW_BIN%;%PATH%"

call :check_tool "%QMAKE%" "qmake"
if errorlevel 1 goto failed
call :check_tool "%WINDEPLOYQT%" "windeployqt"
if errorlevel 1 goto failed
call :check_tool "%MINGW_MAKE%" "mingw32-make"
if errorlevel 1 goto failed

if not exist "%BUILD_ROOT%" mkdir "%BUILD_ROOT%"

if /I "%~1"=="clean" (
    echo Clean build outputs: build\qt and build\package
    if exist "%QT_BUILD%" rmdir /s /q "%QT_BUILD%"
    if errorlevel 1 goto failed
    if exist "%PACKAGE_ROOT%" rmdir /s /q "%PACKAGE_ROOT%"
    if errorlevel 1 goto failed
)

echo Start full build pipeline: C modules -^> Qt UI -^> package

echo [1/4] Build C module: vm_elf_parser
"%MINGW_MAKE%" -C "%ROOT%\vm_elf_parser" all
if errorlevel 1 goto failed

echo [2/4] Build C module: vm_com
"%MINGW_MAKE%" -C "%ROOT%\vm_com" all
if errorlevel 1 goto failed

echo [3/4] Build Qt UI module: vm_ui
if not exist "%QT_BUILD%" mkdir "%QT_BUILD%"
pushd "%QT_BUILD%"
"%QMAKE%" "%PRO_FILE%" CONFIG+=release CONFIG-=debug CONFIG-=debug_and_release CONFIG-=debug_and_release_target -o "%QT_BUILD%\Makefile"
if errorlevel 1 (
    popd
    goto failed
)
"%MINGW_MAKE%" -f "%QT_BUILD%\Makefile" -j4
if errorlevel 1 (
    popd
    goto failed
)
popd

if not exist "%EXE_FILE%" (
    echo Qt output not found: %EXE_FILE%
    goto failed
)
echo Qt build output: %EXE_FILE%

echo [4/4] Package application to build\package\VariableMonitorTool
if exist "%PACKAGE_DIR%" rmdir /s /q "%PACKAGE_DIR%"
if errorlevel 1 goto failed
if not exist "%PACKAGE_DIR%" mkdir "%PACKAGE_DIR%"
copy /Y "%EXE_FILE%" "%PACKAGE_DIR%\" > nul
if errorlevel 1 goto failed
"%WINDEPLOYQT%" --release --compiler-runtime "%PACKAGE_DIR%\VariableMonitorTool.exe"
if errorlevel 1 goto failed

echo Package output: %PACKAGE_DIR%
echo Full build pipeline completed.
echo.
echo Build succeeded. Press any key to close this window.
pause > nul
exit /b 0

:check_tool
if not exist %~1 (
    echo %~2 not found: %~1
    exit /b 1
)
exit /b 0

:failed
echo.
echo Build failed. Please check the messages above.
echo Press any key to close this window.
pause > nul
exit /b 1