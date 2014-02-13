
ExATLCOMps.dll: dlldata.obj ExATLCOM_p.obj ExATLCOM_i.obj
	link /dll /out:ExATLCOMps.dll /def:ExATLCOMps.def /entry:DllMain dlldata.obj ExATLCOM_p.obj ExATLCOM_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del ExATLCOMps.dll
	@del ExATLCOMps.lib
	@del ExATLCOMps.exp
	@del dlldata.obj
	@del ExATLCOM_p.obj
	@del ExATLCOM_i.obj
