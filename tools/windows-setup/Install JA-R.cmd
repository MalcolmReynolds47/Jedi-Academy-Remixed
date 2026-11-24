@echo off
setlocal
title Jedi Academy Remixed
if not exist "%~dp0python\python.exe" (
    echo Extract the entire JA:R ZIP first, then open this file again.
    pause
    exit /b 1
)
"%~dp0python\python.exe" -I -B -X utf8 -u "%~dp0install.py" %*
set "result=%errorlevel%"
if "%~1"=="" pause
exit /b %result%
