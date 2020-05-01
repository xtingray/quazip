make distclean
qmake "PREFIX=/usr/local/quazip" "LIBS+=-lz"
make
