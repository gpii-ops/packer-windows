
# Some code extracted from:
# https://github.com/TheVDIGuys/Windows_10_VDI_Optimize
Function Cleanup() {

    Write-Host 'Cleaning up resources...'

    Write-Host 'Running Dism...'
    
    Start-Process -FilePath dism.exe `
        -ArgumentList "/online","/Cleanup-Image","/restorehealth" `
        -Wait `
        -NoNewWindow `
        -ErrorAction   SilentlyContinue `
        -WarningAction SilentlyContinue

    Start-Process -FilePath dism.exe `
        -ArgumentList "/online","/Cleanup-Image","/StartComponentCleanup","/ResetBase" `
        -Wait `
        -NoNewWindow `
        -ErrorAction   SilentlyContinue `
        -WarningAction SilentlyContinue
    
    Write-Host 'Running Cleanmgr...'

    $strKeyPath   = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\VolumeCaches"
    $strValueName = "StateFlags0065"
    $subkeys      = Get-ChildItem -Path $strKeyPath -Name
    
    ForEach($subkey in $subkeys){
        $null = New-ItemProperty `
            -Path $strKeyPath\$subkey `
            -Name $strValueName `
            -PropertyType DWord `
            -Value 2 `
            -ea SilentlyContinue `
            -wa SilentlyContinue
    }

    $env:HOME = "C:\Users\vagrant"
    $env:DOIT_HOST = "localhost"

    & doitclient.exe wcmd "cleanmgr /sagerun:65"
    
    ForEach($subkey in $subkeys){
        $null = Remove-ItemProperty `
            -Path $strKeyPath\$subkey `
            -Name $strValueName `
            -ea SilentlyContinue `
            -wa SilentlyContinue
    }

    Write-Host 'Uninstalling useless applications...'
    $AppxPackage = @(
        "Microsoft.BingWeather",
        "Microsoft.GetHelp",
        "Microsoft.Getstarted",
        "Microsoft.Messaging",
        "Microsoft.Microsoft3Dviewer",
        "Microsoft.MicrosoftOfficeHub",
        "Microsoft.MicrosoftSolitaireCollection",
        "Microsoft.MicrosoftStickyNotes",
        "Microsoft.MSPaint",
        "Microsoft.Office.OneNote",
        "Microsoft.OneConnect",
        "Microsoft.People",
        "Microsoft.Print3D",
        "Microsoft.SkypeApp",
        "Microsoft.Windows.Photos",
        "Microsoft.WindowsAlarms",
        "Microsoft.WindowsCalculator",
        "Microsoft.WindowsCamera",
        "Microsoft.windowscommunicationsapps",
        "Microsoft.WindowsFeedbackHub",
        "Microsoft.WindowsSoundRecorder",
        "Microsoft.Xbox.TCUI",
        "Microsoft.XboxApp",
        "Microsoft.XboxGameOverlay",
        "Microsoft.XboxGamingOverlay",
        "Microsoft.XboxIdentityProvider",
        "Microsoft.XboxSpeechToTextOverlay",
        "Microsoft.WindowsMaps",
        "Microsoft.ZuneMusic",
        "Microsoft.ZuneVideo"
    )
    
    If ($AppxPackage.Count -gt 0)
    {
        Foreach ($Item in $AppxPackage)
        {
            $Package = "*$Item*"
            Get-AppxPackage                    | Where-Object {$_.PackageFullName -like $Package} | Remove-AppxPackage
            Get-AppxPackage -AllUsers          | Where-Object {$_.PackageFullName -like $Package} | Remove-AppxPackage -AllUsers
            Get-AppxProvisionedPackage -Online | Where-Object {$_.PackageName -like $Package}     | Remove-AppxProvisionedPackage -Online
        }
    }
    #Remove-Item c:\windows\temp\* -Recurse -Force -Exclude *script.ps1*

    Write-Host 'Remove downloaded update files ...'
    Stop-Service -Name wuauserv -Force
    Remove-Item c:\Windows\SoftwareDistribution\Download\* -Recurse -Force
    Start-Service -Name wuauserv
}

Function FillZero() {
    choco install -y sdelete
    choco install -y ultradefrag
    udefrag.exe --optimize --repeat C:
    sdelete -q -z C:
    choco uninstall ultradefrag -y --remove-dependencies
    choco uninstall sdelete -y --remove-dependencies
}

Cleanup
FillZero
