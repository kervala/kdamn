set UPLOADER="C:\Program Files\RedmineUploader\redmineuploader.exe"

call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x86

set QTDIR=D:/External/vc10/qt-5.4.0-static-32

rmdir /s /q package_x32

mkdir package_x32

cd package_x32
cmake .. -DCMAKE_BUILD_TYPE=Release -G"NMake Makefiles"

nmake package

move *.exe ..

cd ..

rem pause

call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64

set QTDIR=D:/External/vc10/qt-5.4.0-static-64

rmdir /s /q package_x64

mkdir package_x64

cd package_x64
cmake .. -DCMAKE_BUILD_TYPE=Release -G"NMake Makefiles"

nmake package

move *.exe ..

cd ..

rem pause

%UPLOADER% kdamn 0.9  kdamn-*.exe

pause
