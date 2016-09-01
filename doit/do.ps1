Param(
  [Parameter(Mandatory=$True)]
  [String]
  [Alias("c")]
  $command
)

$env:HOME = "C:\Users\vagrant"
$env:DOIT_HOST = "localhost"

cd C:\vagrant
doitclient.exe wcmd "refreshenv && ${command}"
exit $lastExitCode
