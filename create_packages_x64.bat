call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64

set QTDIR=D:/External/vc10/qt-5.4.0-static-64

set PATH=%PATH%;C:\Program files (x86)\CMake\bin

rmdir /s /q package_x64

mkdir package_x64

cd package_x64
cmake .. -DCMAKE_BUILD_TYPE=Release -G"NMake Makefiles"

nmake package

mv *.exe ..

cd ..

pause
