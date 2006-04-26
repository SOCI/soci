COMPILER = g++
CXXFLAGS = -Wall -pedantic -Wno-long-long
INCLUDEDIRS = -I.. -I../../../core
LIBDIRS = -L.. -L../../../core
LIBS = -lsoci-core -lsoci-empty


test-empty : test-empty.cpp
	${COMPILER} -o $@ $? ${CXXFLAGS} ${INCLUDEDIRS} ${LIBDIRS} ${LIBS}


clean :
	rm -f test-empty
