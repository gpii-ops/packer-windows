# This script set the configure files of the browsers to avoid
# initial dialogs at the first run

Function Set-Browsers-Defaults {
$mozpath = 'C:\Program Files'
$chromepath = 'C:\Program Files'
# Mozilla firefox
$content = @'
// WKS Mozilla Firefox Lockdown
// Disable updater
lockPref("app.update.enabled", false);
// Make absolutely sure it is really off
lockPref("app.update.auto", false);
lockPref("app.update.mode", 0);
lockPref("app.update.service.enabled", false);
// Disable Add-ons compatibility checking
clearPref("extensions.lastAppVersion"); 
// Don't show 'know your rights' on first run
pref("browser.rights.3.shown", true);
// Don't show WhatsNew on first run after every update
pref("browser.startup.homepage_override.mstone","ignore");
// Don't show Windows 10 splash screen on first run
pref("browser.usedOnWindows10", true);
// Set default homepage
lockPref("browser.startup.homepage","https://www.google.com");
// Disable the internal PDF viewer
lockPref("pdfjs.disabled", true);
// Disable the flash to javascript converter
lockPref("shumway.disabled", true);
// Don't ask to install the Flash plugin
pref("plugins.notifyMissingFlash", false);
// Disable plugin checking
lockPref("plugins.hide_infobar_for_outdated_plugin", true);
clearPref("plugins.update.url");
// Disable health reporter
lockPref("datareporting.healthreport.service.enabled", false);
// Disable all data upload (Telemetry and FHR)
lockPref("datareporting.policy.dataSubmissionEnabled", false);
// Disable crash reporter
lockPref("toolkit.crashreporter.enabled", false);
Components.classes["@mozilla.org/toolkit/crash-reporter;1"].getService(Components.interfaces.nsICrashReporter).submitReports = false; 
// Disable default browser check
lockPref("browser.shell.checkDefaultBrowser", false);
// Delete history on exit
lockPref("browser.history_expire_days", 0);
lockPref("browser.history_expire_days.mirror", 0);
lockPref("browser.formfill.enable", false);
lockPref("browser.download.manager.retention", 0);
lockPref("network.cookie.cookieBehavior", 0);
lockPref("network.cookie.lifetimePolicy", 2);
// Disable password manager
lockPref("signon.rememberSignons", false);
lockPref("pref.privacy.disable_button.view_passwords", true);
// Disable themes
lockPref("config.lockdown.disable_themes", true);
// Enable Java Plugin
lockPref("security.enable_java", true);
// Automatically enable extensions
lockPref("extensions.autoDisableScopes", 0);
'@
Set-Content -Value $content -Path "$mozpath\Mozilla Firefox\mozilla.cfg"

$content = @'
[XRE]
EnableProfileMigrator=false
'@
Set-Content -Value $content -Path "$mozpath\Mozilla Firefox\browser\override.ini"

$content = @'
pref("general.config.filename", "mozilla.cfg");
pref("general.config.obscure_value", 0);
'@
Set-Content -Value $content -Path "$mozpath\Mozilla Firefox\defaults\pref\local-settings.js"

# Google Chrome
$content = @'
{
  "homepage": "http://www.google.com", 
  "homepage_is_newtabpage": false,
  "browser": { 
    "show_home_button": true 
  }, 
  "bookmark_bar": { 
    "show_on_all_tabs": false 
  }, 
  "distribution": { 
    "skip_first_run_ui": true, 
    "import_bookmarks": false,
    "import_search_engine": true, 
    "import_history": false, 
    "create_all_shortcuts": true, 
    "do_not_launch_chrome": true, 
    "make_chrome_default": true, 
    "make_chrome_default_for_user": true
  }, 
  "first_run_tabs": [
    "http://www.google.com"
  ]
}
'@
$Arch = (Get-Process -Id $PID).StartInfo.EnvironmentVariables["PROCESSOR_ARCHITECTURE"];
if ($Arch -eq 'amd64') {
    $chromepath = 'C:\Program Files (x86)'
}
Set-Content -Value $content -Path $chromepath\Google\Chrome\Application\master_preferences

# Internet Explorer
New-Item -Path "HKLM:\Software\Policies\Microsoft" -Name "Internet Explorer" -Force
New-Item -Path "HKLM:\Software\Policies\Microsoft\Internet Explorer" -Name "Main" -Force
New-ItemProperty -Path "HKLM:\Software\Policies\Microsoft\Internet Explorer\Main" -Name "DisableFirstRunCustomize"  -Value 1 -PropertyType "DWord" -Force
}

Function Install-Chocolatey-packages {

  choco install sysinternals -y
  choco install curl -y
  choco install jre8 -y
  choco install git.install -params '"/GitOnlyOnPath"' -y
  choco install firefox -y
  choco install googlechrome -y
  choco install vcbuildtools -ia "/InstallSelectableItems VisualCppBuildTools_ATLMFC_SDK;VisualCppBuildTools_NETFX_SDK" -y
  choco install windows-sdk-10-version-1809-all --version=10.0.17763.1 -y
  choco install microsoft-build-tools -y
  choco install visualcppbuildtools -y

  [Environment]::SetEnvironmentVariable("VCTargetsPath", "C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\v140", [System.EnvironmentVariableTarget]::Machine)

}

Function Install-Firewall-rules {

  netsh advfirewall firewall add rule name="Nodejs" dir=in action=allow program="c:\program files\nodejs\node.exe" enable=yes
  netsh advfirewall firewall add rule name="Nodejs 32-bit" dir=in action=allow program="c:\program files (x86)\nodejs\node.exe" enable=yes
  netsh advfirewall firewall add rule name="Nodejs commandline" dir=in action=allow program="c:\programdata\chocolatey\lib\nodejs.commandline\tools\node.exe" enable=yes
  
}

Function Enable-UAC {

  New-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System\" -Name "EnableLUA" -PropertyType "DWORD" -Value "1" -Force
  New-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System\" -Name "ConsentPromptBehaviorAdmin" -PropertyType "DWORD" -Value "0" -Force
  New-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System\" -Name "ConsentPromptBehaviorUser" -PropertyType "DWORD" -Value "0" -Force
  New-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System\" -Name "PromptOnSecureDesktop" -PropertyType "DWORD" -Value "0" -Force

}

Install-Chocolatey-packages
Install-Firewall-rules
Set-Browsers-Defaults
Enable-UAC
