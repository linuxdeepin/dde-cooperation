Set shell = Wscript.createobject("wscript.shell")
Set objWMIService = GetObject("winmgmts:\\.\root\cimv2")
Set backProcess = objWMIService.ExecQuery("Select * from Win32_Process where Name = 'cooperation-daemon.exe'")
if backProcess.Count = 0 Then
    Set bsProcesses = objWMIService.ExecQuery("Select * from Win32_Process where Name = 'barriers.exe'")
    For Each objProccess in bsProcesses
        objProccess.Terminate()
    Next
    Set bcProcesses = objWMIService.ExecQuery("Select * from Win32_Process where Name = 'barrierc.exe'")
    For Each objProccess in bcProcesses
        objProccess.Terminate()
    Next
    ' Wscript.Echo "backend daemon is not running."
    a = shell.run (".\cooperation-daemon.exe",0)
    Wscript.Sleep 500
    Set checkProcess = objWMIService.ExecQuery("Select * from Win32_Process where Name = 'cooperation-daemon.exe'")
    if checkProcess.Count = 0 Then
        Wscript.Echo "Failed to start backend daemon: cooperation-daemon.exe"
        Wscript.Quit
    else
        ' Wscript.Echo "cooperation-daemon.exe is started successfully."
    end if
else
    ' Wscript.Echo "backend daemon is already running."
end if

b = shell.run(".\dde-cooperation.exe",0)
