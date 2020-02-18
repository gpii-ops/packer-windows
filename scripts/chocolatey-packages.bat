set choco_binary=c:\programdata\chocolatey\bin\choco.exe

%choco_binary% feature enable -n allowEmptyChecksums

%choco_binary% install sysinternals -y
%choco_binary% install curl -y
%choco_binary% install jre8 -y
%choco_binary% install maven -y
%choco_binary% install jdk8 -y
%choco_binary% install git.install -params '"/GitOnlyOnPath"' -y
%choco_binary% install firefox -y
%choco_binary% install googlechrome -y
%choco_binary% install vcbuildtools -ia "/InstallSelectableItems VisualCppBuildTools_ATLMFC_SDK;VisualCppBuildTools_NETFX_SDK" -y
%choco_binary% install windows-sdk-10-version-1809-all --version=10.0.17763.1 -y
%choco_binary% install microsoft-build-tools -y
%choco_binary% install visualcppbuildtools -y
setx /M VCTargetsPath "C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\v140"
%choco_binary% Install-ChocolateyEnvironmentVariable "JAVA_HOME" "C:\Program Files\Java\jdk1.8.0_211"
%choco_binary% Install-ChocolateyEnvironmentVariable "MAVEN_HOME" "C:\Users\vagrant\Downloads\apache-maven-3.6.3-bin\apache-maven-3.6.3"
