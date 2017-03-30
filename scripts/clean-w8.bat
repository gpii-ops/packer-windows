dism.exe /online /cleanup-Image /restoreHealth
sfc /scannow
dism.exe /Online /Cleanup-Image /StartComponentCleanup
exit /b 0