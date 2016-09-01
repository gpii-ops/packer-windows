Param(
  [Parameter(Mandatory=$True)] [String]$command
)

$env:HOME = "C:\Users\vagrant"
$env:DOIT_HOST = "localhost"

cd c:\vagrant
doitclient.exe wcmd "refreshenv && ${command}"
