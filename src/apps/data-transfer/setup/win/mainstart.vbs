Set shell = Wscript.createobject("wscript.shell")

a = shell.run (".\daemon.bat",0)
b = shell.run (".\frontend.bat",0)