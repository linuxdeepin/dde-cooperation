; 该脚本使用 HM VNISEdit 脚本编辑器向导产生

; 安装程序初始定义常量
!define PRODUCT_NAME "uos-win-assistant"
!define PRODUCT_VERSION "1.0.4"
!define PRODUCT_PUBLISHER "统信软件"
!define PRODUCT_WEB_SITE "http://www.uniontech.com"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

!define UosServiceFile "UOSAssistantSvc"
!define APP_NAME "uos-assistant"
!define SENDFILE_NAME "ShellSendFile"
!define APP_ICON "logo"

SetCompressor lzma

; ------ MUI 现代界面定义 (1.67 版本以上兼容) ------
!include "MUI.nsh"
!include "LogicLib.nsh"

; MUI 预定义常量
!define MUI_ABORTWARNING
!define MUI_ICON "logo.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

Var SMFolder

; 欢迎页面
!insertmacro MUI_PAGE_WELCOME
; 许可协议页面
!insertmacro MUI_PAGE_LICENSE "Licence.txt"
; 安装目录选择页面
!insertmacro MUI_PAGE_DIRECTORY
;开始菜单
!insertmacro MUI_PAGE_STARTMENU Suite $SMFolder
; 安装过程页面
!insertmacro MUI_PAGE_INSTFILES
; 安装完成页面
!insertmacro MUI_PAGE_FINISH

; 安装卸载过程页面
!insertmacro MUI_UNPAGE_INSTFILES

; 安装界面包含的语言设置
!insertmacro MUI_LANGUAGE "SimpChinese"

; 安装提示
LangString VERSION_IS_WRONG ${LANG_SIMPCHINESE} "当前软件版本不支持升级！"
LangString SOFTWARE_IS_RUNNING ${LANG_SIMPCHINESE} "软件正在运行，请先关闭软件！"

; 安装预释放文件
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS
; ------ MUI 现代界面定义结束 ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${PRODUCT_NAME}_${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES\UOSPA"
ShowInstDetails show
ShowUnInstDetails show
BrandingText "UOS电脑助手"
RequestExecutionLevel admin

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  AccessControl::GrantOnFile \
		"$INSTDIR" "(BU)" "GenericRead + GenericWrite"
	Pop $0
  SetOverwrite ifnewer
  File /r "..\Files\*.*"
  CreateDirectory "$INSTDIR\Files"
  CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\${APP_NAME}.exe" ;"" "$INSTDIR\${APP_ICON}.ico"
  WriteRegStr HKCR "AllFilesystemObjects\shell\发送至协同设备\command" "" "$INSTDIR\${SENDFILE_NAME}.exe %1"
SectionEnd

Section
!insertmacro MUI_STARTMENU_WRITE_BEGIN Suite
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\${APP_NAME}.exe"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\uninst.exe"
!insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

;Section -AdditionalIcons
;  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\uninst.exe"
;SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  ExecWait '"$INSTDIR\${UosServiceFile}.exe" install'
  ExecWait '"$INSTDIR\${UosServiceFile}.exe" start'
SectionEnd

/******************************
 *  以下是安装程序的卸载部分  *
 ******************************/

Section Uninstall
	ExecWait '"$INSTDIR\${UosServiceFile}.exe" stop'
	ExecWait '"$INSTDIR\${UosServiceFile}.exe" uninstall'
	
  Delete "$INSTDIR\uninst.exe"

  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
  Delete "$DESKTOP\${PRODUCT_NAME}.lnk"

  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"
  RMDir ""

  RMDir /r "$INSTDIR\sqldrivers"
  RMDir /r "$INSTDIR\QtQuick.2"
  RMDir /r "$INSTDIR\QtQuick"
  RMDir /r "$INSTDIR\QtQml"
  RMDir /r "$INSTDIR\QtGraphicalEffects"
  RMDir /r "$INSTDIR\Qt"
  RMDir /r "$INSTDIR\platforms"
  RMDir /r "$INSTDIR\imageformats"

  RMDir /r /REBOOTOK "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
	DeleteRegKey HKCR "AllFilesystemObjects\shell\发送至协同设备"
	DeleteRegKey HKCU "SOFTWARE\UnionTech"
  SetAutoClose true
SectionEnd

#-- 根据 NSIS 脚本编辑规则，所有 Function 区段必须放置在 Section 区段之后编写，以避免安装程序出现未可预知的问题。--#
;
Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "您确实要完全移除 $(^Name) ，及其所有的组件？" IDYES +2
  Abort
  FindProcDLL::FindProc "${APP_NAME}.exe"
  Pop $R0
  IntCmp $R0 1 running no_run
  running:
	  KillProcDLL::KillProc "${APP_NAME}.exe"
  	Sleep 1000
  	FindProcDLL::FindProc "${APP_NAME}.exe"
  	Pop $R0
  	IntCmp $R0 1 srunning no_run
  	srunning:
  	  Quit
	no_run:
FunctionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) 已成功地从您的计算机移除。"
FunctionEnd

Function CheckInstalled
	ReadRegStr $0 ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion"
	StrLen $1 $0
	IntCmp $1 0 0 0 +2
	goto end
	MessageBox MB_OKCANCEL|MB_ICONSTOP "检测到已经安装过UOS电脑助手，$\r$\n点击“确定”卸载$\r$\n点击“取消”退出安装" IDOK 0 IDCANCEL +3
	ReadRegStr $2 ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString"
	ExecWait '$2'
	Abort
	end:
FunctionEnd

; 检查软件运行
Function .onInit
	Call CheckInstalled
	FindProcDLL::FindProc "${APP_NAME}.exe"
	Pop $R0
	IntCmp $R0 1 running no_run
		running:
		MessageBox MB_OKCANCEL|MB_ICONSTOP "检测到${PRODUCT_NAME}正在运行。$\r$\n点击“确定”关闭程序，继续安装。$\r$\n点击“取消”退出安装程序。" IDOK label_ok IDCANCEL label_cancel
		label_ok:
		KillProcDLL::KillProc "${APP_NAME}.exe"
		Sleep 1000
		FindProcDLL::FindProc "${APP_NAME}.exe"
		Pop $R0
		IntCmp $R0 1 srunning no_run
		srunning:
		  Quit
		Goto no_run
		label_cancel:
			Quit
	no_run:
FunctionEnd
