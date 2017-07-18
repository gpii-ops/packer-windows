:: Windows 7 SP1 require KB3102810 hotfix
:: Installing and searching for updates is slow and high CPU usage occurs in Windows 7.
:: https://support.microsoft.com/en-us/kb/3102810

@echo off

if "%PROCESSOR_ARCHITECTURE%"=="AMD64" goto DL64
echo "32 bits detected downloading hotfix"
@powershell -NoProfile -ExecutionPolicy Bypass -Command "((new-object net.webclient).DownloadFile('https://download.microsoft.com/download/A/0/9/A09BC0FD-747C-4B97-8371-1A7F5AC417E9/Windows6.1-KB3102810-x86.msu', 'C:\Windows\Temp\Windows6.1-KB3102810-x64.msu'))"
@powershell -NoProfile -ExecutionPolicy Bypass -Command "((new-object net.webclient).DownloadFile('https://download.microsoft.com/download/0/A/F/0AFB5316-3062-494A-AB78-7FB0D4461357/windows6.1-KB976932-X86.exe', 'C:\Windows\Temp\sp1.exe'))"
goto END

:DL64
echo "64 bits detected downloading hotfix"
@powershell -NoProfile -ExecutionPolicy Bypass -Command "((new-object net.webclient).DownloadFile('https://download.microsoft.com/download/F/A/A/FAABD5C2-4600-45F8-96F1-B25B137E3C87/Windows6.1-KB3102810-x64.msu', 'C:\Windows\Temp\Windows6.1-KB3102810-x64.msu'))"
@powershell -NoProfile -ExecutionPolicy Bypass -Command "((new-object net.webclient).DownloadFile('https://download.microsoft.com/download/0/A/F/0AFB5316-3062-494A-AB78-7FB0D4461357/windows6.1-KB976932-X64.exe', 'C:\Windows\Temp\sp1.exe'))"

:END
set hotfix="C:\Windows\Temp\Windows6.1-KB3102810-x64.msu"
if not exist %hotfix% goto :eof

:: get windows version
for /f "tokens=2 delims=[]" %%G in ('ver') do (set _version=%%G)
for /f "tokens=2,3,4 delims=. " %%G in ('echo %_version%') do (set _major=%%G& set _minor=%%H& set _build=%%I)

:: 6.1
if %_major% neq 6 goto :eof
if %_minor% lss 1 goto :eof

@echo on
echo "Installing Service Pack 1"
C:\Windows\Temp\sp1.exe /quiet /nodialog /norestart

@powershell -NoProfile -ExecutionPolicy Bypass -Command "Set-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run' -Name 'InstallWindowsUpdates' -Value 'C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -File a:\win-updates.ps1 -MaxUpdatesPerCycle 100 -RestartRequired 1'"

timeout /t 2

echo "Installing Hotfix"
start /wait wusa "%hotfix%" /quiet
