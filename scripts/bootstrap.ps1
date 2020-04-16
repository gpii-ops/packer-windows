# This script is called from the answerfile

# You cannot enable Windows PowerShell Remoting on network connections that are set to Public
# http://msdn.microsoft.com/en-us/library/windows/desktop/aa370750(v=vs.85).aspx
# http://blogs.msdn.com/b/powershell/archive/2009/04/03/setting-network-location-to-private.aspx


$networkListManager = [Activator]::CreateInstance([Type]::GetTypeFromCLSID([Guid]'{DCB00C01-570F-4A9B-8D69-199FDBA5723B}'))
$connections = $networkListManager.GetNetworkConnections()

$connections |foreach {
    Write-Host "Setting network config"
    $_.GetNetwork().GetName() + 'category was previously set to' + $_.GetNetwork().GetCategory()
    $_.GetNetwork().SetCategory(1)
    $_.GetNetwork().GetName() + 'change to ' + $_.GetNetwork().GetCategory()
}

# https://github.com/TheVDIGuys/Windows_10_VDI_Optimize/blob/master/1803/1803_VDI_Configuration.ps1
Function Optimize-Network {
  #region Network Optimization
  # LanManWorkstation optimizations
  New-ItemProperty -Path "HKLM:\System\CurrentControlSet\Services\LanmanWorkstation\Parameters\" -Name "DisableBandwidthThrottling" -PropertyType "DWORD" -Value "1" -Force
  New-ItemProperty -Path "HKLM:\System\CurrentControlSet\Services\LanmanWorkstation\Parameters\" -Name "FileInfoCacheEntriesMax" -PropertyType "DWORD" -Value "1024" -Force
  New-ItemProperty -Path "HKLM:\System\CurrentControlSet\Services\LanmanWorkstation\Parameters\" -Name "DirectoryCacheEntriesMax" -PropertyType "DWORD" -Value "1024" -Force
  New-ItemProperty -Path "HKLM:\System\CurrentControlSet\Services\LanmanWorkstation\Parameters\" -Name "FileNotFoundCacheEntriesMax" -PropertyType "DWORD" -Value "1024" -Force
  New-ItemProperty -Path "HKLM:\System\CurrentControlSet\Services\LanmanWorkstation\Parameters\" -Name "DormantFileLimit" -PropertyType "DWORD" -Value "256" -Force
}

Function Enable-WinRM {
  Write-Host "Enable WinRM"
  netsh advfirewall firewall set rule group="remote administration" new enable=yes
  netsh advfirewall firewall add rule name="Open Port 5985" dir=in action=allow protocol=TCP localport=5985
  
  winrm quickconfig -q
  winrm quickconfig -transport:http
  winrm set winrm/config '@{MaxTimeoutms="7200000"}'
  winrm set winrm/config/winrs '@{MaxMemoryPerShellMB="0"}'
  winrm set winrm/config/winrs '@{MaxProcessesPerShell="0"}'
  winrm set winrm/config/winrs '@{MaxShellsPerUser="0"}'
  winrm set winrm/config/service '@{AllowUnencrypted="true"}'
  winrm set winrm/config/service/auth '@{Basic="true"}'
  winrm set winrm/config/client/auth '@{Basic="true"}'
  
  net stop winrm
  sc.exe config winrm start= auto
  net start winrm
}

Function Install-Chocolatey {
  $chocoExePath = 'C:\ProgramData\Chocolatey\bin'

  if ($($env:Path).ToLower().Contains($($chocoExePath).ToLower())) {
    Write-Host "Chocolatey found in PATH, skipping install..."
    Exit
  } else {
    Write-Host "Installing Chocolatey..."
    # Add to system PATH
    $systemPath = [Environment]::GetEnvironmentVariable('Path',[System.EnvironmentVariableTarget]::Machine)
    $systemPath += ';' + $chocoExePath
    [Environment]::SetEnvironmentVariable("PATH", $systemPath, [System.EnvironmentVariableTarget]::Machine)
    
    # Update local process' path
    $userPath = [Environment]::GetEnvironmentVariable('Path',[System.EnvironmentVariableTarget]::User)
    if($userPath) {
      $env:Path = $systemPath + ";" + $userPath
    } else {
      $env:Path = $systemPath
    }
    
    # Run the installer
    iex ((new-object net.webclient).DownloadString('https://chocolatey.org/install.ps1'))
  }
  choco install -y vcredist140
}

# Disable windows updates
$RunningAsAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
if ($RunningAsAdmin)
{

	$Updates = (New-Object -ComObject "Microsoft.Update.AutoUpdate").Settings

	if ($Updates.ReadOnly -eq $True) { Write-Error "Cannot update Windows Update settings due to GPO restrictions." }

	else {
		$Updates.NotificationLevel = 1 #Disabled
		$Updates.Save()
		$Updates.Refresh()
		Write-Output "Automatic Windows Updates disabled."
	}
} else {	
  Write-Warning "Must be executed in Administrator level shell."
	Write-Warning "Script Cancelled!" 
}

Optimize-Network
Get-WmiObject -Class Win32_UserAccount -Filter "name = 'vagrant'" | Set-WmiInstance -Arguments @{PasswordExpires = 0 }
Install-Chocolatey
Enable-WinRM
