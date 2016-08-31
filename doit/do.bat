@ECHO OFF

set HOME=C:\Users\vagrant
set DOIT_HOST=localhost

cd c:\vagrant
call refreshenv 1> nul
doitclient.exe wcmd %*
