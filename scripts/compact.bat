@echo off

if "%PROCESSOR_ARCHITECTURE%"=="AMD64" goto DL64
echo "32 bits detected"
set URL7ZIP=http://www.7-zip.org/a/7z1604.msi
set URLDEFRAG=http://downloads.sourceforge.net/project/ultradefrag/stable-release/7.0.2/ultradefrag-portable-7.0.2.bin.i386.zip
set DEFRAGVER=7.0.2.i386
goto END

:DL64
echo "64 bits detected"
set URL7ZIP=http://www.7-zip.org/a/7z920-x64.msi
set URLDEFRAG=http://downloads.sourceforge.net/project/ultradefrag/stable-release/6.1.0/ultradefrag-portable-6.1.0.bin.amd64.zip
set DEFRAGVER=6.1.0.amd64

:END
if not exist "C:\Windows\Temp\7z.msi" (
	powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%URL7ZIP%', 'C:\Windows\Temp\7z.msi')" <NUL
)
msiexec /qb /i C:\Windows\Temp\7z.msi

if not exist "C:\Windows\Temp\ultradefrag.zip" (
	powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%URLDEFRAG%', 'C:\Windows\Temp\ultradefrag.zip')" <NUL
)

if not exist "C:\Windows\Temp\ultradefrag-portable-%DEFRAGVER%\udefrag.exe" (
	cmd /c ""C:\Program Files\7-Zip\7z.exe" x C:\Windows\Temp\ultradefrag.zip -oC:\Windows\Temp"
)

if not exist "C:\Windows\Temp\SDelete.zip" (
  powershell -Command "(New-Object System.Net.WebClient).DownloadFile('http://download.sysinternals.com/files/SDelete.zip', 'C:\Windows\Temp\SDelete.zip')" <NUL
)

if not exist "C:\Windows\Temp\sdelete.exe" (
	cmd /c ""C:\Program Files\7-Zip\7z.exe" x C:\Windows\Temp\SDelete.zip -oC:\Windows\Temp"
)

msiexec /qb /x C:\Windows\Temp\7z.msi

net stop wuauserv
rmdir /S /Q C:\Windows\SoftwareDistribution\Download
mkdir C:\Windows\SoftwareDistribution\Download
net start wuauserv

cmd /c C:\Windows\Temp\ultradefrag-portable-%DEFRAGVER%\udefrag.exe --optimize --repeat C:

cmd /c %SystemRoot%\System32\reg.exe ADD HKCU\Software\Sysinternals\SDelete /v EulaAccepted /t REG_DWORD /d 1 /f
cmd /c C:\Windows\Temp\sdelete.exe -q -z C:
