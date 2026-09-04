@echo off
setlocal
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
start "" "%~dp0build\DiskSpaceAnalyzer.exe"
endlocal
