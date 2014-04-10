To compile the bootloader yourself, you can make use the following script, but you have to adapt the path in the first line to your environment:

set BASEDIR=C:\arduino-0017\hardware
set DIRAVRUTIL=%BASEDIR%\tools\avr\utils\bin
set DIRAVRBIN=%BASEDIR%\tools\avr\bin
set DIRAVRAVR=%BASEDIR%\tools\avr\avr\bin
set DIRLIBEXEC=%BASEDIR%\tools\avr\libexec\gcc\avr\4.3.2
set OLDPATH=%PATH%
@path %DIRAVRUTIL%;%DIRAVRBIN%;%DIRAVRAVR%;%DIRLIBEXEC%;%PATH%
%DIRAVRUTIL%\make.exe ng
%DIRAVRUTIL%\make.exe atmega328
@path %OLDPATH%