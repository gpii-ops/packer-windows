Set-MpPreference -DisableRealtimeMonitoring $true
New-ItemProperty -Path 'hklm:\SOFTWARE\Policies\Microsoft\Windows Defender' -Name "DisableAntiSpyware" -Value 1 -PropertyType "DWORD"
New-ItemProperty -Path 'hklm:\SOFTWARE\Policies\Microsoft\Windows Defender' -Name "DisableRoutinelyTakingAction" -Value 1 -PropertyType "DWORD"