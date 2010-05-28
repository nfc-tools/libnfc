set OLDPATH=%PATH%
set PATH=%PATH%;"c:\program files\makemsi"
rmdir /s /q out
call "%ProgramFiles%\MakeMSI\mm.cmd" "libnfc.mm"
if errorlevel 1 goto failed

:success
echo success
goto doneall

:failed
echo failed

:doneall
set PATH=%OLDPATH%
