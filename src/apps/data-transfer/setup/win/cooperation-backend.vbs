Set shell = Wscript.createobject("wscript.shell")
Set objWMIService = GetObject("winmgmts:\\.\root\cimv2")
Set backProcess = objWMIService.ExecQuery("Select * from Win32_Process where Name = 'cooperation-daemon.exe'")
if backProcess.Count = 0 Then
    ' Wscript.Echo "backend daemon is not running."
    a = shell.run (".\cooperation-daemon.exe",0)
end if