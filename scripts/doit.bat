@echo off

if "%PROCESSOR_ARCHITECTURE%"=="AMD64" goto DL64
echo "Copying DoIt client for Windows for 32 bits"
cd c:\doit
copy doitclient_x86.exe C:\Windows\doitclient.exe

echo "Copying DoIt server for 32 bits"
cd c:\doit
copy doit_x86.exe C:\Windows\doit.exe
goto END

:DL64
echo "Copying DoIt client for Windows for 64 bits"
cd c:\doit
copy doitclient_x64.exe C:\Windows\doitclient.exe

echo "Copying DoIt server for 64 bits"
cd c:\doit
copy doit_x64.exe C:\Windows\doit.exe

:END
echo "Copying required files"
cd c:\doit
copy doit-secret C:\Windows
copy doitrc C:\Users\vagrant\.doitrc
copy do.ps1 C:\Windows
copy doit-server.bat "C:\Users\vagrant\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup"
copy mount-drive.bat "C:\Users\vagrant\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup"

echo "Setting required environment variables"
setx /M DOIT_SERVER localhost
setx /M HOME C:\Users\vagrant

echo "Setting firewall rules"
netsh advfirewall firewall add rule name="Allow DoIt server" dir=in action=allow program="C:\Windows\doit.exe"
netsh advfirewall firewall add rule name="Allow DoIt client" dir=out action=allow program="C:\Windows\doitclient.exe"

echo "Cleaning up build area"
cd c:\
rmdir /s /q c:\doit
