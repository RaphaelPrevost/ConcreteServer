;CONCRETE SERVER SETUP

;--------------------------------
;Modern UI

    !include "MUI.nsh"

;--------------------------------
;General

    ; name and settings
    Name "Concrete Server"
    ;Icon "mammouth.ico"
    WindowIcon on
    ;XPStyle on
    OutFile "ConcreteSetup.exe"
    BrandingText "Copyright © 2005-2012 BGA://Buro.asia Limited"
    SetCompressor /solid lzma
    CRCCheck on
    ShowInstDetails show

    ; versioning
    VIProductVersion "0.3.1.0"

    ; setup informations
    VIAddVersionKey "ProductName" "Concrete Server for Windows NT"
    VIAddVersionKey "Comments" "Multiprotocol IP Server"
    VIAddVersionKey "CompanyName" "BGA://Buro.asia Limited"
    VIAddVersionKey "LegalCopyright" "Copyright (c) 2012 BGA://Buro.asia Limited"
    VIAddVersionKey "FileDescription" "Concrete Server Setup"
    VIAddVersionKey "FileVersion" "0.3.1"

    ; default installation folder
    InstallDir "$PROGRAMFILES\buro.asia\concrete"

;--------------------------------
;Interface Settings

    !define MUI_WELCOMEFINISHPAGE_BITMAP "logo.bmp"
    !define MUI_UNWELCOMEFINISHPAGE_BITMAP "logo.bmp"
    !define MUI_HEADERIMAGE
    !define MUI_HEADERIMAGE_BITMAP "concrete.bmp"
    ;!define MUI_ICON "mammouth.ico"
    ;!define MUI_UNICON

;--------------------------------
;Pages

    !define MUI_FINISHPAGE_RUN_TEXT "Install Concrete NT Service"
    !define MUI_FINISHPAGE_RUN "$INSTDIR\concrete.exe /i"

    !insertmacro MUI_PAGE_WELCOME
    !insertmacro MUI_PAGE_LICENSE "..\COPYING"
    !insertmacro MUI_PAGE_DIRECTORY
    !insertmacro MUI_PAGE_INSTFILES
    !insertmacro MUI_PAGE_FINISH

    !insertmacro MUI_UNPAGE_WELCOME
    !insertmacro MUI_UNPAGE_CONFIRM
    !insertmacro MUI_UNPAGE_INSTFILES
    !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

    !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;NSIS library

    !include Library.nsh

;--------------------------------
;Concrete Server installation

Section "Install" SecConcrete

    ; set the installation folder
    SetOutPath "$INSTDIR"

    ; copy the program files
    File "concrete.exe"
    File "libconcrete.dll"
    File "libiconv-2.dll"
    File "libxml2.dll"
    File "libmysql.dll"
    File "msvcp60.dll"
    File "pthreadGC2.dll"
    File "zlib1.dll"
    File "..\concrete.xml"
    File "..\concrete.dtd"
    ;File "mammouth.ico"

    SetOutPath "$INSTDIR\plugins"
    File "dialmsn.so"

    ; set the path
    WriteRegStr HKLM \
    "SYSTEM\CurrentControlSet\Services\Concrete\Parameters" \
    "ConcretePath" "$INSTDIR"

    ; add uninstall informations to the registry
    WriteRegStr HKLM \
    "Software\Microsoft\Windows\CurrentVersion\Uninstall\Concrete" \
    "DisplayName" "Concrete Server"
    WriteRegStr HKLM \
    "Software\Microsoft\Windows\CurrentVersion\Uninstall\Concrete" \
    "DisplayVersion" "0.3.1"
    WriteRegStr HKLM \
    "Software\Microsoft\Windows\CurrentVersion\Uninstall\Concrete" \
    "Publisher" "BGA://Buro.asia Limited"
    WriteRegStr HKLM \
    "Software\Microsoft\Windows\CurrentVersion\Uninstall\Concrete" \
    "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr HKLM \
    "Software\Microsoft\Windows\CurrentVersion\Uninstall\Concrete" \
    "DisplayIcon" "$INSTDIR\mammouth.ico"
    WriteRegDword HKLM \
    "Software\Microsoft\Windows\CurrentVersion\Uninstall\Concrete" \
    "NoModify" 0x00000001
    WriteRegDword HKLM \
    "Software\Microsoft\Windows\CurrentVersion\Uninstall\Concrete" \
    "NoRepair" 0x00000001
    WriteRegStr HKLM \
    "Software\Microsoft\Windows\CurrentVersion\Uninstall\Concrete" \
    "DisplayIcon" "$INSTDIR\mammouth.ico"

    ; create directories and links
    CreateDirectory "$SMPROGRAMS\Concrete Server"
    CreateShortCut "$SMPROGRAMS\Concrete Server\Uninstall.lnk" \
    "$INSTDIR\Uninstall.exe"
    CreateShortCut "$SMPROGRAMS\Concrete Server\Interactive console.lnk" \
    "$INSTDIR\concrete.exe"
    CreateShortCut "$SMPROGRAMS\Concrete Server\Install NT service.lnk" \
    "$INSTDIR\concrete.exe /i"
    CreateShortCut "$SMPROGRAMS\Concrete Server\Uninstall NT service.lnk" \
    "$INSTDIR\concrete.exe /u"
    CreateShortCut "$SMPROGRAMS\Concrete Server\Start NT service.lnk" \
    "$INSTDIR\concrete.exe /s"
    CreateShortCut "$SMPROGRAMS\Concrete Server\Stop NT service.lnk" \
    "$INSTDIR\concrete.exe /q"

    ; create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

SectionEnd

;--------------------------------
;Mammouth Server uninstallation

Section "Uninstall"

    ; delete program files
    Delete "$INSTDIR\concrete.exe"
    Delete "$INSTDIR\libconcrete.dll"
    Delete "$INSTDIR\libiconv-2.dll"
    Delete "$INSTDIR\libxml2.dll"
    Delete "$INSTDIR\libmysql.dll"
    Delete "$INSTDIR\msvcp60.dll"
    Delete "$INSTDIR\pthreadGC2.dll"
    Delete "$INSTDIR\zlib1.dll"
    Delete "$INSTDIR\concrete.xml"
    Delete "$INSTDIR\concrete.dtd"
    Delete "$INSTDIR\plugins\dialmsn.so"
    ;Delete "$INSTDIR\mammouth.ico"

    ; delete links
    Delete "$SMPROGRAMS\Concrete Server\Uninstall.lnk"
    Delete "$SMPROGRAMS\Concrete Server\Interactive console.lnk"
    Delete "$SMPROGRAMS\Concrete Server\Install NT service.lnk"
    Delete "$SMPROGRAMS\Concrete Server\Uninstall NT service.lnk"
    Delete "$SMPROGRAMS\Concrete Server\Start NT service.lnk"
    Delete "$SMPROGRAMS\Concrete Server\Stop NT service.lnk"
    RmDir "$SMPROGRAMS\Concrete Server"

    ; delete the uninstaller itself
    Delete "$INSTDIR\Uninstall.exe"

    ; delete the installation folder
    RmDir "$INSTDIR\plugins"
    RmDir "$INSTDIR"

    ; remove registry keys
    DeleteRegKey HKLM \
    "SYSTEM\CurrentControlSet\Services\Concrete"
    DeleteRegKey HKLM \
    "Software\Microsoft\Windows\CurrentVersion\Uninstall\Concrete"

SectionEnd

;--------------------------------
;Descriptions

    ;Language strings
    LangString DESC_SecConcrete ${LANG_ENGLISH} "Concrete Server"

    ;Assign language strings to sections
    !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
        !insertmacro MUI_DESCRIPTION_TEXT ${SecConcrete} $(DESC_SecConcrete)
    !insertmacro MUI_FUNCTION_DESCRIPTION_END
