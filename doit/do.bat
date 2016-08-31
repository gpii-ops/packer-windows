@ECHO OFF

set HOME=C:\Users\vagrant
set DOIT_HOST=localhost

cd c:\vagrant
call refreshenv
doitclient.exe wcmd %*
