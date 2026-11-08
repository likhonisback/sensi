@echo off
title Building Sensi Pro Android APK...
cd /d "%~dp0"
powershell.exe -ExecutionPolicy Bypass -File "%~dp0build_android.ps1"
pause
