:: Update for Windows 8.1 (KB3102812)
:: Installing and searching for updates is slow and high CPU usage occurs in Windows 8.1 and Windows Server 2012 R2
:: Additional updates are required first to install this hotfix at the end of the installation

:: @echo off

if "%PROCESSOR_ARCHITECTURE%"=="AMD64" goto DL64
echo "32 bits detected downloading hotfix"
powershell -NoProfile -ExecutionPolicy Bypass -Command "((new-object net.webclient).DownloadFile('https://download.microsoft.com/download/4/E/C/4EC66C83-1E15-43FD-B591-63FB7A1A5C04/Windows8.1-KB2919355-x86.msu', 'C:\Windows\Temp\Windows8.1-KB2919355.msu'))"
powershell -NoProfile -ExecutionPolicy Bypass -Command "((new-object net.webclient).DownloadFile('https://download.microsoft.com/download/9/D/A/9DA6C939-9E65-4681-BBBE-A8F73A5C116F/Windows8.1-KB2919442-x86.msu', 'C:\Windows\Temp\Windows8.1-KB2919442.msu'))"
powershell -NoProfile -ExecutionPolicy Bypass -Command "((new-object net.webclient).DownloadFile('https://download.microsoft.com/download/4/C/1/4C130775-6445-4330-9DC9-DEE9768E834A/Windows8.1-KB3102812-x86.msu', 'C:\Windows\Temp\Windows8.1-KB3102812.msu'))"
goto END

:DL64
echo "64 bits detected downloading hotfix"
powershell -NoProfile -ExecutionPolicy Bypass -Command "((new-object net.webclient).DownloadFile('https://download.microsoft.com/download/D/B/1/DB1F29FC-316D-481E-B435-1654BA185DCF/Windows8.1-KB2919355-x64.msu', 'C:\Windows\Temp\Windows8.1-KB2919355.msu'))"
powershell -NoProfile -ExecutionPolicy Bypass -Command "((new-object net.webclient).DownloadFile('https://download.microsoft.com/download/C/F/8/CF821C31-38C7-4C5C-89BB-B283059269AF/Windows8.1-KB2919442-x64.msu', 'C:\Windows\Temp\Windows8.1-KB2919442.msu'))"
powershell -NoProfile -ExecutionPolicy Bypass -Command "((new-object net.webclient).DownloadFile('https://download.microsoft.com/download/8/7/1/87133323-C696-4BD4-A464-BBDFA2A00A37/Windows8.1-KB3102812-x64.msu', 'C:\Windows\Temp\Windows8.1-KB3102812.msu'))"

:END
set hotfix1="C:\Windows\Temp\Windows8.1-KB2919442.msu"
set hotfix2="C:\Windows\Temp\Windows8.1-KB2919355.msu"
set hotfix3="C:\Windows\Temp\Windows8.1-KB3102812.msu"

if not exist %hotfix1% goto :eof
if not exist %hotfix2% goto :eof
if not exist %hotfix3% goto :eof

dir C:\Windows\Temp\Win*

:: get windows version
for /f "tokens=2 delims=[]" %%G in ('ver') do (set _version=%%G) 
for /f "tokens=2,3,4 delims=. " %%G in ('echo %_version%') do (set _major=%%G& set _minor=%%H& set _build=%%I) 

:: 6.2 or 6.3
if %_major% neq 6 goto :eof
if %_minor% lss 2 goto :eof
if %_minor% gtr 3 goto :eof

::@echo on


echo "Installing Hotfix KB2919442"
start /wait wusa "%hotfix1%" /quiet /norestart
echo "Installing Hotfix KB2919355"
start /wait wusa "%hotfix2%" /quiet /norestart
echo "Installing Hotfix KB3102812"
start /wait wusa "%hotfix3%" /quiet /norestart

powershell -NoProfile -ExecutionPolicy Bypass -Command "Set-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run' -Name 'InstallWindowsUpdates' -Value 'C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -File a:\win-updates.ps1 -MaxUpdatesPerCycle 30'"
