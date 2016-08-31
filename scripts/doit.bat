echo "Building DoIt client for Windows"
call "C:\Program Files (x86)\Microsoft Visual C++ Build Tools\vcbuildtools_msbuild.bat"
cd c:\doit\client
msbuild doitclient.sln
copy c:\doit\client\Release\doitclient.exe C:\Windows

echo "Building DoIt server"
call "C:\Program Files (x86)\Microsoft Visual C++ Build Tools\vcbuildtools.bat" amd64
cd c:\doit\server
nmake /f Makefile.vc
copy doit.exe C:\Windows

echo "Copying required files"
cd c:\doit
copy doit-secret C:\Windows
copy doitrc C:\Users\vagrant\.doitrc
copy do.bat C:\Windows
copy doit-server.bat "C:\Users\vagrant\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup"

echo "Setting required environment variables"
setx DOIT_SERVER 127.0.0.1
setx HOME C:\Users\vagrant

echo "Cleaning up build area"
cd c:\
rmdir /s /q c:\doit
