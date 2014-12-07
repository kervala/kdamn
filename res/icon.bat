@echo off

set DIR=%CD%

echo Current directory is %DIR%

cd "C:\Program Files (x86)\Inkscape"

c:

call:convert_desktop icon icon %DIR%
call:convert %DIR%\header %DIR%\header 57 1

convert %DIR%\header.png -units PixelsPerInch -density 72 -alpha off BMP3:%DIR%\header.bmp

del %DIR%\header.png

echo "Done."

pause
goto:eof

:convert
set SRC=%~1
set DST=%~2
set SIZE=%~3
rem 0 is transparent and 1 is opaque
set ALPHA=%~4
inkscape -z -f %SRC%.svg -e %DST%.png -h %SIZE% -y %ALPHA% 2> nul
goto:eof

:convert_desktop
set FILENAME1=%~1
set FILENAME2=%~2
set SRC_ICON=%~3\%FILENAME1%
set DST_ICON=%~3\%FILENAME2%

echo Converting %FILENAME1% for Desktop...
call:convert %SRC_ICON% %DST_ICON%16x16 16 0
call:convert %SRC_ICON% %DST_ICON%22x22 22 0
call:convert %SRC_ICON% %DST_ICON%24x24 24 0
call:convert %SRC_ICON% %DST_ICON%32x32 32 0
call:convert %SRC_ICON% %DST_ICON%48x48 48 0
call:convert %SRC_ICON% %DST_ICON%64x64 64 0
call:convert %SRC_ICON% %DST_ICON%128x128 128 0
call:convert %SRC_ICON% %DST_ICON%256x256 256 0
call:convert %SRC_ICON% %DST_ICON%512x512 512 0

convert %DST_ICON%256x256.png %DST_ICON%48x48.png %DST_ICON%32x32.png %DST_ICON%16x16.png %DST_ICON%.ico

goto:eof
