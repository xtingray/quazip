sudo rm -rf build
sudo rm -rf /usr/local/quazip
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local/quazip -DCMAKE_CXX_FLAGS="-lz" ..
make
sudo make install
sudo mv /usr/local/quazip/include/QuaZip-Qt5-1.3/quazip /usr/local/quazip/include/ 
sudo rm -rf /usr/local/quazip/include/QuaZip-Qt5-1.3
