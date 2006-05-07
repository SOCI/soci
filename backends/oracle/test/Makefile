# The following variable is specific to this backend and its correct
# values might depend on your environment - feel free to set it accordingly.

ORACLEINCLUDEDIR = -I/u01/oracle/product/10.1.0/rdbms/public 
ORACLELIBDIR = -L/u01/oracle/product/10.1.0/lib 
ORACLELIBS = -lclntsh 

# The rest of the Makefile is indepentent of the target environment.

COMPILER = g++
CXXFLAGS = -Wall -pedantic -Wno-long-long
INCLUDEDIRS = -I.. -I../../../core ${ORACLEINCLUDEDIR}
LIBDIRS = -L.. -L../../../core ${ORACLELIBDIR}
LIBS = -lsoci-core -lsoci-oracle ${ORACLELIBS}


test-oracle : test-oracle.cpp
	${COMPILER} -o $@ $? ${CXXFLAGS} ${INCLUDEDIRS} ${LIBDIRS} ${LIBS}


clean :
	rm -f *.o test-oracle
