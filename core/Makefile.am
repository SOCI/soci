# $Id: Makefile.am,v 1.5 2007/03/02 09:45:18 msobczak Exp $
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
	values.cpp

socidir = $(includedir)/soci

soci_HEADERS = \
	blob.h \
	into-row.h \
	into-type.h \
	once-temp-type.h \
	prepare-temp-type.h \
	procedure.h \
	ref-counted-prepare-info.h \
	ref-counted-statement.h \
	row.h \
	rowid.h \
	rowset.h \
	session.h \
	soci-backend.h \
	soci-config.h \
	soci-platform.h \
	soci.h \
	statement.h \
	type-conversion.h \
	type-holder.h \
	type-ptr.h \
	use-type.h \
	values.h

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
