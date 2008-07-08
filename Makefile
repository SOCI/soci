build_libs :
	cd build/unix; `cat local/tcl` build.tcl core core-so mysql mysql-so postgresql postgresql-so oracle oracle-so

tests :
	cd build/unix; `cat local/tcl` build.tcl mysql-test oracle-test postgresql-test

install :
	cd build/unix; `cat local/tcl` install.tcl

uninstall :
	cd build/unix/local; /bin/sh uninstall.sh

clean :
	find . -name '*.o' | xargs rm
	find . -name '*.a' | xargs rm
	find . -name '*.so' | xargs rm
	rm -f src/backends/mysql/test/test-mysql
	rm -f src/backends/oracle/test/test-oracle
	rm -f src/backends/postgresql/test/test-postgresql
	rm -rf build/unix/include build/unix/lib build/unix/tests
