set choco_binary=c:\programdata\chocolatey\bin\choco.exe

%choco_binary% install sysinternals -y
%choco_binary% install curl -y
%choco_binary% install jre8 -y
%choco_binary% install git.install -params '"/GitOnlyOnPath"' -y
%choco_binary% install firefox -y
%choco_binary% install googlechrome -y
