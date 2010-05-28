;----------------------------------------------------------------------------
;    MODULE NAME:   LIBNFC.MM
;
;        $Author:   USER "rogerb"  $
;      $Revision: 1267 $
;          $Date:   02 Jun 2006 17:10:46  $
;       $Logfile:   C:/DBAREIS/Projects.PVCS/Win32/MakeMsi/TryMe.mm.pvcs  $
;
; DESCRIPTION
; ~~~~~~~~~~~
; This is a simple sample/test MSI. Takes about 30 seconds to build and
; validate on my AMD 3200.
;
; Any line within this file that begins with ";" can be ignored as its
; only a comment so there are only 3 important lines in this file:
;
;   1. #include "ME.MMH"
;   2. <$DirectoryTree Key="INSTALLDIR" ...
;   3. <$Files "TryMe.*" DestDir="INSTALLDIR">
;----------------------------------------------------------------------------

; #define? COMPANY_PRODUCT_ICON     ..\win32\libnfc.ico  ;; override from company.mmh
#define? UISAMPLE_DIALOG_FILE_dlgbmp	nfcleft.bmp     ;; override uisample.mmh
#define? UISAMPLE_BLINE_TEXT           www.libnfc.org
#define? COMPANY_WANT_TO_INSTALL_DOCUMENTATION   N

;--- Include MAKEMSI support (with my customisations and MSI branding) ------
#define VER_FILENAME.VER  libnfc.Ver      ;;I only want one VER file for all samples! (this line not actually required in "tryme.mm")
#include "ME.MMH"

;--- Want to debug (not common) ---------------------------------------------
;#debug on
;#Option DebugLevel=^NONE, +OpSys^


;--- Define default location where file should install and add files --------
<$DirectoryTree Key="INSTALLDIR" Dir="[ProgramFilesFolder]libnfc-1.3.4" CHANGE="\" PrimaryFolder="Y">
<$DirectoryTree Key="INSTALLDIR2" Dir="[INSTALLDIR]bin" >
<$DirectoryTree Key="INSTALLDIR3" Dir="[INSTALLDIR]lib" >
<$DirectoryTree Key="INSTALLDIR4" Dir="[INSTALLDIR]include" >
<$DirectoryTree Key="INSTALLDIR5" Dir="[INSTALLDIR4]nfc" >
<$Files "..\bin\nfc-list.exe" DestDir="INSTALLDIR2" >
<$Files "..\bin\nfc-poll.exe" DestDir="INSTALLDIR2" >
<$Files "..\bin\nfc-relay.exe" DestDir="INSTALLDIR2" >
<$Files "..\bin\nfc-emulate.exe" DestDir="INSTALLDIR2" >
<$Files "..\bin\nfc-mfultralight.exe" DestDir="INSTALLDIR2" >
<$Files "..\bin\nfc-mfclassic.exe" DestDir="INSTALLDIR2" >
<$Files "..\bin\nfcip-initiator.exe" DestDir="INSTALLDIR2" >
<$Files "..\bin\nfcip-target.exe" DestDir="INSTALLDIR2" >
<$Files "..\bin\nfc.dll" DestDir="SystemFolder" >
<$Files "..\bin\nfc.lib" DestDir="INSTALLDIR3" >
<$Files "..\..\include\nfc\nfc.h" DestDir="INSTALLDIR5" >
<$Files "..\..\include\nfc\nfc-messages.h" DestDir="INSTALLDIR5" >
<$Files "..\..\include\nfc\nfc-types.h" DestDir="INSTALLDIR5" >
