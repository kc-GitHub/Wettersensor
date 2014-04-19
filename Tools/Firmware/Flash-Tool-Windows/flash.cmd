::**************
::* Flash-Tool *
::**************

@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

if %1!==! goto explainUsage & goto end
if %2!==! goto explainUsage & goto end

::**************************
::* convert hexfile to bin *
::**************************
bin\hex2bin -c %2 > NUL
for /f "tokens=1,2 delims=. " %%a in ("%2") do set fileBasename=%%a&set fileExt=%%b

if %3!==! if %4!==! (
	goto useDefaultValues
)

::**************************
::* Set the default values *
::**************************
set "defHmId1=AB
set "defHmId2=CD
set "defHmId3=EF
set "defSerialNumber=HB0Default"

echo.

::*****************************
::* Check the for valid HM-ID *
::*****************************
for /f "tokens=1,2,3 delims=: " %%a in ("%3") do set hmId1=%%a&set hmId2=%%b&set hmId3=%%c
call :checkHex hmId1
call :checkHex hmId2
call :checkHex hmId3

set "hmIdError="
IF (%hmId1%) == () set hmIdError=1
IF (%hmId2%) == () set hmIdError=1
IF (%hmId3%) == () set hmIdError=1
IF defined hmIdError (
	call :explainUsage "The entered HM-ID "%3" is invalid. Format: XX:XX:XX. Each X must be 0-9 or A-F."
	goto end
) else (
	set "hmId=\x%defHmId1%\x%defHmId2%\x%defHmId3%"
)

::*********************************
::* Check the valid serial number *
::*********************************
set "serialNr=%4"
set "serialNrLen=0"
call :strLen serialNr serialNrLen
if NOT %serialNrLen% EQU 10 (
	set "serialNr=~~~"
)

set "res="&for /f "delims=0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdrefgijklmnopqrstuvwxyz" %%i in ("%serialNr%") do set "res=%%i"
IF NOT %res%!==! (
	call :explainUsage "The serial number must contains 10 characters 0-9 or A-Z."
	goto end
)


::**********************************************************
::* Write user defined HM-ID and serial number to bin-file *
::**********************************************************
bin\sed -b -e "s/\(%defSerialNumber%\)\(.*\)\(\x%defHmId1%\x%defHmId2%\x%defHmId3%\)/%serialNr%\2\x%hmId1%\x%hmId2%\x%hmId3%/" %fileBasename%.bin > %fileBasename%.tmp
del %fileBasename%.bin
ren %fileBasename%.tmp %fileBasename%.bin

echo Start flashing device with HM-ID: %hmId1%:%hmId2%:%hmId3% and serial number: %serialNr% 


:useDefaultValues

::*******************************
::* Begin flashing with avrdude *
::*******************************
if %serialNr%!==! (
	echo Start flashing device with predefined HM-ID and serial number.
)

bin\avrdude.exe -Cbin\avrdude.conf -patmega328p -carduino -P %1 -b57600 -D -Uflash:w:%fileBasename%.bin:r 
del %fileBasename%.bin

goto :end



::*********************************************************************************************************
::* At this point some subroutines defined                                                                *
::*********************************************************************************************************

::**************************
::* Write out param errors *
::**************************
: explainUsage <errorText> (
    setlocal EnableDelayedExpansion
    
	IF NOT %1!==! (
		echo Error: %1
		echo.
	)
	
	echo Usage: flash.bat ^<COMX^> ^<hexfile^> [^<HM-ID^> ^<Serial^>]
    exit /b
)

::**********************************
::* Count the length of a variable *
::**********************************
:strLen <stringVar> <resultVar> (   
::    setlocal EnableDelayedExpansion

	set "s=!%~1!#"
    set "len=0"
    for %%P in (4096 2048 1024 512 256 128 64 32 16 8 4 2 1) do (
        if "!s:~%%P,1!" NEQ "" ( 
            set /a "len+=%%P"
            set "s=!s:~%%P!"
        )
    )
) ( endlocal
    set "%~2=%len%"
    exit /b
)

::***************************************
::* Check a variable if is a hex number *
::***************************************
:checkHex <txtVar> (   
::    setlocal EnableDelayedExpansion
    setlocal

	set "res=!%~1!"
	set "var="&for /f "delims=0123456789ABCDEFabcdef" %%i in ("%res%") do set "var=%%i"

	if %var%!==! (
		rem
	) else (
		set "res="
	)
	
	set "resLen=0"
	call :strLen res resLen

	if %resLen% GTR 2 (
		set "res="
	)

	set "%~1=%res%"
    exit /b
)

::*********************************************************************************************************
::* The end.                                                                                              *
::*********************************************************************************************************
:end
echo.
