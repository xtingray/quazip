rmdir /Q /s build
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX="C:/Quazip" -DQUAZIP_USE_QT_ZLIB=ON  ..
make
rmdir /Q /s C:\Quazip 
mkdir C:\Quazip
mkdir C:\Quazip\bin
mkdir C:\Quazip\include
mkdir C:\Quazip\include\quazip
copy quazip\libquazip1-qt5.dll C:\Quazip\bin
copy C:\devel\sources\quazip\quazip\*h C:\Quazip\include\quazip
