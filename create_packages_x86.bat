call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x86

set QTDIR=D:/External/vc10/qt-5.3.1-static-32

set PATH=%PATH%;C:\Program files (x86)\CMake\bin

rmdir /s /q package_x32

mkdir package_x32

cd package_x32
cmake .. -DCMAKE_BUILD_TYPE=Release -G"NMake Makefiles"

nmake package

mv *.exe ..

cd ..

pause
