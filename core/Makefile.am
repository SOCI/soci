# $Id: Makefile.am,v 1.14 2008/05/20 21:24:11 msobczak Exp $
# 
# Copyright (C) 2006, Mateusz Loskot
#
# Author: Mateusz Loskot <mateusz@loskot.net>
#
AM_CXXFLAGS = -fPIC $(BOOST_CPPFLAGS)

SUBDIRS = test

lib_LTLIBRARIES	= lib@SOCI_CORE_LIBRARY_NAME@.la

lib@SOCI_CORE_LIBRARY_NAME@_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@ -no-undefined @LD_LIB@

lib@SOCI_CORE_LIBRARY_NAME@_la_SOURCES = \
	backend-loader.cpp \
	blob.cpp \
	connection-pool.cpp \
	error.cpp \
	into-type.cpp \
	once-temp-type.cpp \
	prepare-temp-type.cpp \
	procedure.cpp \
	ref-counted-prepare-info.cpp \
	ref-counted-statement.cpp \
	row.cpp \
	rowid.cpp \
	session.cpp \
	soci-simple.cpp \
	statement.cpp \
	transaction.cpp \
	use-type.cpp \
	values.cpp

socidir = $(includedir)/soci

soci_HEADERS = \
	backend-loader.h \
	blob.h \
	blob-exchange.h \
	boost-gregorian-date.h \
	boost-fusion.h \
	boost-optional.h \
	boost-tuple.h \
	connection-pool.h \
	error.h \
	exchange-traits.h \
	into.h \
	into-type.h \
	once-temp-type.h \
	prepare-temp-type.h \
	procedure.h \
	ref-counted-prepare-info.h \
	ref-counted-statement.h \
	row.h \
	row-exchange.h \
	rowid.h \
	rowid-exchange.h \
	rowset.h \
	session.h \
	soci-backend.h \
	soci-config.h \
	soci-platform.h \
	soci-simple.h \
	soci.h \
	statement.h \
	transaction.h \
	type-conversion.h \
	type-conversion-traits.h \
	type-holder.h \
	type-ptr.h \
	use.h \
	use-type.h \
	values.h \
	values-exchange.h

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
