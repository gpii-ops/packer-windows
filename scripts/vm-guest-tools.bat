@echo off

if "%PROCESSOR_ARCHITECTURE%"=="AMD64" goto DL64
echo "32 bits detected"
set URL7ZIP="http://www.7-zip.org/a/7z1604.msi"
goto END

:DL64
echo "64 bits detected"
set URL7ZIP="http://www.7-zip.org/a/7z920-x64.msi"


:END
if not exist "C:\Windows\Temp\7z.msi" (
    powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%URL7ZIP%', 'C:\Windows\Temp\7z.msi')" <NUL
)
msiexec /qb /i C:\Windows\Temp\7z.msi

if "%PACKER_BUILDER_TYPE%" equ "virtualbox-iso" goto :virtualbox
goto :done

:virtualbox

:: There needs to be Oracle CA (Certificate Authority) certificates installed in order
:: to prevent user intervention popups which will undermine a silent installation.
cmd /c certutil -addstore -f "TrustedPublisher" A:\oracle-cert.cer

move /Y C:\Users\vagrant\VBoxGuestAdditions.iso C:\Windows\Temp
cmd /c ""C:\Program Files\7-Zip\7z.exe" x C:\Windows\Temp\VBoxGuestAdditions.iso -oC:\Windows\Temp\virtualbox"
cmd /c C:\Windows\Temp\virtualbox\VBoxWindowsAdditions.exe /S

:done
msiexec /qb /x C:\Windows\Temp\7z.msi
