COMPILER = g++
CXXFLAGS = -Wall -pedantic -Wno-long-long
INCLUDEDIRS = -I/u01/app/oracle/product/10.1.0/Db_1/rdbms/public
LIBDIRS = -L/u01/app/oracle/product/10.1.0/Db_1/lib
LIBS = -lclntsh

test : test.cpp soci.cpp
	${COMPILER} -o $@ test.cpp soci.cpp ${CXXFLAGS} ${INCLUDEDIRS} ${LIBDIRS} ${LIBS}
