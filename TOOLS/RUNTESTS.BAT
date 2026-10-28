@echo off
rem 26.10.15.1 12:00PM
rem -------------------------------------------------------------------------
rem RUNTESTS.BAT - builds and runs the BBS-independent unit tests (x86).
rem
rem Run from anywhere; it finds the project root from its own location and
rem sets up the Visual Studio x86 toolchain itself. Each test prints PASS
rem and exits 0. Output goes to BUILD\TESTS (git-ignored).
rem -------------------------------------------------------------------------
setlocal
set "ROOT=%~dp0.."
set "OUT=%ROOT%\BUILD\TESTS"
if not exist "%OUT%" mkdir "%OUT%"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
rem (for /f over vswhere returns nothing under some shells - use a file)
"%VSWHERE%" -latest -products * -property installationPath >"%OUT%\VSDIR.TXT"
set /p VSDIR=<"%OUT%\VSDIR.TXT"
call "%VSDIR%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul 2>&1 || exit /b 2

set FAIL=0
call :one DATETEST SBDATE
call :one FMTTEST SBFMT
call :one SYSTEST SBSYS
if %FAIL%==0 (echo ALL TESTS PASSED) else (echo TESTS FAILED)
exit /b %FAIL%

:one
cl /nologo /J /W3 /I"%ROOT%\SRC\SNBBSS" /Fo"%OUT%\\" /Fe"%OUT%\%1.EXE" "%ROOT%\TESTS\%1.C" "%ROOT%\SRC\SNBBSS\%2.C" >"%OUT%\%1.BUILD.LOG" || (echo %1: BUILD FAILED & type "%OUT%\%1.BUILD.LOG" & set FAIL=1 & goto :eof)
<nul set /p=%1:
"%OUT%\%1.EXE" || set FAIL=1
goto :eof
