echo "Building DoIt client for Windows"
cd c:\doit\client
copy c:\doit\client\Release\doitclient_x64.exe C:\Windows\doitclient.exe

echo "Building DoIt server"
cd c:\doit\server
copy doit_x64.exe C:\Windows\doit.exe

echo "Copying required files"
cd c:\doit
copy doit-secret C:\Windows
copy doitrc C:\Users\vagrant\.doitrc
copy do.ps1 C:\Windows
copy doit-server.bat "C:\Users\vagrant\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup"

echo "Setting required environment variables"
setx /M DOIT_SERVER localhost
setx /M HOME C:\Users\vagrant

echo "Setting firewall rules"
netsh advfirewall firewall add rule name="Allow DoIt server" dir=in action=allow program="C:\Windows\doit.exe"
netsh advfirewall firewall add rule name="Allow DoIt client" dir=out action=allow program="C:\Windows\doitclient.exe"

echo "Cleaning up build area"
cd c:\
rmdir /s /q c:\doit
