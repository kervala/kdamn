@echo off

set COMPILER=vc12
set UNIXDIR=D:\Projects\packaging\unix
set CURRENTDIR=%cd%

set QTVERSION=5.9.2

cd %UNIXDIR%
call setenv.bat %COMPILER% x86

cd %CURRENTDIR%

set QTDIR=D:/External/%COMPILER%/qt-%QTVERSION%-static-64

rmdir /s /q package_x64

mkdir package_x64

cd package_x64
cmake .. -DCMAKE_BUILD_TYPE=Release -G"NMake Makefiles"

nmake package

move *.exe ..

cd ..

pause

set QTVERSION=5.6.3

cd %UNIXDIR%
call setenv.bat %COMPILER% x86

cd %CURRENTDIR%

set QTDIR=D:/External/%COMPILER%/qt-%QTVERSION%-static-32

rmdir /s /q package_x32

mkdir package_x32

cd package_x32
cmake .. -DCMAKE_BUILD_TYPE=Release -G"NMake Makefiles"

nmake package

move *.exe ..

cd ..

pause
