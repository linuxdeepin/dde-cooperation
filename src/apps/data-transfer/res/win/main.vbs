kill = False
Set shell = Wscript.createobject("wscript.shell")
Set objWMIService = GetObject("winmgmts:\\.\root\cimv2")
Set backProcess = objWMIService.ExecQuery("Select * from Win32_Process where Name = 'cooperation-daemon.exe'")
if backProcess.Count = 0 Then
    ' Wscript.Echo "backend daemon is not running."
    a = shell.run (".\cooperation-daemon.exe",0)
    Wscript.Sleep 500
    Set checkProcess = objWMIService.ExecQuery("Select * from Win32_Process where Name = 'cooperation-daemon.exe'")
    if checkProcess.Count = 0 Then
        Wscript.Echo "Failed to start backend daemon: cooperation-daemon.exe"
        Wscript.Quit
    else
        ' Wscript.Echo "cooperation-daemon.exe is started successfully."
        kill = True
    end if
else
    ' Wscript.Echo "backend daemon is already running."
end if

b = shell.run(".\deepin-data-transfer.exe",0)

' 检测自身进程退出，如果是自己拉起来的后端，则杀掉
Do While True
    Set colProcess = objWMIService.ExecQuery("Select * from Win32_Process where Name = 'deepin-data-transfer.exe'")
    if colProcess.Count = 0 Then
        if kill = True Then
            Set daemonProcesses = objWMIService.ExecQuery("Select * from Win32_Process where Name = 'cooperation-daemon.exe'")
            For Each objProccess in daemonProcesses
                objProccess.Terminate()
            Next
        end if
        Wscript.Quit
    end if

    Wscript.Sleep 2000
Loop