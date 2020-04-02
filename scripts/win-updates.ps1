param($global:RestartRequired=0,
        $global:MoreUpdates=0,
        $global:MaxCycles=10)

function Check-ContinueRestartOrEnd() {
    $RegistryKey = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run"
    $RegistryEntry = "InstallWindowsUpdates"
    switch ($global:RestartRequired) {
        0 {
            $prop = (Get-ItemProperty $RegistryKey).$RegistryEntry
            if ($prop) {
                Write-Host "Restart Registry Entry Exists - Removing It"
                Remove-ItemProperty -Path $RegistryKey -Name $RegistryEntry -ErrorAction SilentlyContinue
            }

            Write-Host "No Restart Required"
            Check-WindowsUpdates

            if (($global:MoreUpdates -eq 1) -and ($script:Cycles -le $global:MaxCycles)) {
                Install-WindowsUpdates
            } elseif ($script:Cycles -gt $global:MaxCycles) {
                Write-Host "Exceeded Cycle Count - Stopping"
                Invoke-Expression "a:\bootstrap.ps1"
			} else {
                Write-Host "Done Installing Windows Updates"
                Invoke-Expression "a:\bootstrap.ps1"
            }
        }
        1 {
            $prop = (Get-ItemProperty $RegistryKey).$RegistryEntry
            if (-not $prop) {
                Write-Host "Restart Registry Entry Does Not Exist - Creating It"
                Set-ItemProperty -Path $RegistryKey -Name $RegistryEntry -Value "C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -File $($script:ScriptPath)"
            } else {
                Write-Host "Restart Registry Entry Exists Already"
            }

            Write-Host "Restart Required - Restarting..."
            Restart-Computer
        }
        default {
            Write-Host "Unsure If A Restart Is Required"
            break
        }
    }
}

function Install-WindowsUpdates() {
    $script:Cycles++
    Get-WUInstall -MicrosoftUpdate -AcceptAll -Download -Install -IgnoreReboot

    if (Get-WURebootStatus -Silent) {
        $global:RestartRequired=1
    } else {
        $global:RestartRequired=0
    }

    Check-ContinueRestartOrEnd
}

function Check-WindowsUpdates() {
    Write-Host "Checking For Windows Updates"
    
    $script:SearchResult = Get-WindowsUpdate
    if ($SearchResult.Count -ne 0) {
        $global:MoreUpdates=1
    } else {
        Write-Host 'There are no applicable updates'
        $global:RestartRequired=0
        $global:MoreUpdates=0
        Invoke-Expression "a:\bootstrap.ps1"
    }
}


$script:ScriptName = $MyInvocation.MyCommand.ToString()
$script:ScriptPath = $MyInvocation.MyCommand.Path
$script:Cycles = 0

Check-WindowsUpdates
if ($global:MoreUpdates -eq 1) {
    Install-WindowsUpdates
} else {
    Check-ContinueRestartOrEnd
}

