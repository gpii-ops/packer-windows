Write-Host 'Clearing CleanMgr.exe automation settings.'
Get-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\VolumeCaches\*' -Name StateFlags0001 -ErrorAction SilentlyContinue | Remove-ItemProperty -Name StateFlags0001 -ErrorAction SilentlyContinue

Write-Host 'Enabling Update Cleanup. This is done automatically in Windows 10 via a scheduled task.'
New-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\VolumeCaches\Update Cleanup' -Name StateFlags0001 -Value 2 -PropertyType DWord

Write-Host 'Enabling Temporary Files Cleanup.'
New-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\VolumeCaches\Temporary Files' -Name StateFlags0001 -Value 2 -PropertyType DWord

Write-Host 'Starting CleanMgr.exe...'
Start-Process -FilePath CleanMgr.exe -ArgumentList '/sagerun:1' -WindowStyle Hidden -Wait

Write-Host 'Waiting for CleanMgr and DismHost processes. Second wait neccesary as CleanMgr.exe spins off separate processes.'
Get-Process -Name cleanmgr,dismhost -ErrorAction SilentlyContinue | Wait-Process

$UpdateCleanupSuccessful = $false
if (Test-Path $env:SystemRoot\Logs\CBS\DeepClean.log) {
    $UpdateCleanupSuccessful = Select-String -Path $env:SystemRoot\Logs\CBS\DeepClean.log -Pattern 'Total size of superseded packages:' -Quiet
}
