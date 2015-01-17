call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64

set QTDIR=D:/External/vc10/qt-5.4.0-static-64

rmdir /s /q package_x64

mkdir package_x64

cd package_x64
cmake .. -DCMAKE_BUILD_TYPE=Release -G"NMake Makefiles"

nmake package

move *.exe ..

cd ..

pause
