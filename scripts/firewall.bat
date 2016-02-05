netsh advfirewall firewall add rule name="Nodejs" dir=in action=allow program="c:\program files\nodejs\node.exe" enable=yes
netsh advfirewall firewall add rule name="Nodejs 32-bit" dir=in action=allow program="c:\program files (x86)\nodejs\node.exe" enable=yes
netsh advfirewall firewall add rule name="Nodejs commandline" dir=in action=allow program="c:\programdata\chocolatey\lib\nodejs.commandline\tools\node.exe" enable=yes
