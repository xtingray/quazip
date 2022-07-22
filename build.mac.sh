sudo rm -rf build
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
sudo mv /usr/local/include/QuaZip-Qt5-1.3/quazip /usr/local/include/ 
sudo rm -rf /usr/local/include/QuaZip-Qt5-1.3
