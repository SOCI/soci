# $Id: Makefile.am,v 1.7 2007/06/21 21:34:31 msobczak Exp $
# 
# Copyright (C) 2006, Mateusz Loskot
#
# Author: Mateusz Loskot <mateusz@loskot.net>
#
AM_CXXFLAGS = -fPIC

SUBDIRS = test

lib_LTLIBRARIES	= lib@SOCI_CORE_LIBRARY_NAME@.la

lib@SOCI_CORE_LIBRARY_NAME@_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@

lib@SOCI_CORE_LIBRARY_NAME@_la_SOURCES = \
	blob.cpp \
	into-type.cpp \
	once-temp-type.cpp \
	prepare-temp-type.cpp \
	procedure.cpp \
	ref-counted-prepare-info.cpp \
	ref-counted-statement.cpp \
	row.cpp \
	rowid.cpp \
	session.cpp \
	statement.cpp \
	use-type.cpp \
	values.cpp \
	error.cpp

socidir = $(includedir)/soci

soci_HEADERS = \
	blob.h \
	blob-exchange.h \
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
	soci.h \
	statement.h \
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
