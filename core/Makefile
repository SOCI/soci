COMPILER = g++
CXXFLAGS = -Wall -pedantic -Wno-long-long


libsoci-core.a : soci.o
	ar rv $@ $?
	rm *.o

soci.o : soci.cpp
	${COMPILER} -c $? ${CXXFLAGS}


shared : soci.cpp
	${COMPILER} -fPIC -c $? ${CXXFLAGS}
	${COMPILER} -shared -o libsoci-core.so soci.o
	rm *.o


clean :
	rm -f libsoci-core.a libsoci-core.so
