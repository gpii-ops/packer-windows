@ECHO OFF

set HOME=C:\Users\vagrant
set DOIT_HOST=localhost

cd c:\vagrant

doitclient.exe wcmd "refreshenv && %*"
