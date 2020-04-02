
# Installing Guest Additions
Write-Host 'Installing Guest Additions'

if (Test-Path d:\VBoxWindowsAdditions.exe) {
  Write-Host "Mounting Drive with VBoxWindowsAdditions"
  certutil -addstore -f "TrustedPublisher" D:\cert\vbox-sha1.cer
  & d:\VBoxWindowsAdditions.exe /S
  Write-Host "Sleeping for 30 seconds so we are sure the tools are installed before reboot"
  Start-Sleep -s 30
}
if (Test-Path e:\VBoxWindowsAdditions.exe) {
  Write-Host "Mounting Drive with VBoxWindowsAdditions"
  certutil -addstore -f "TrustedPublisher" E:\cert\vbox-sha1.cer
  & E:\VBoxWindowsAdditions.exe /S
  Write-Host "Sleeping for 30 seconds so we are sure the tools are installed before reboot"
  Start-Sleep -s 30
}
