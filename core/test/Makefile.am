# $Id: Makefile.am,v 1.2 2007/06/23 01:51:22 mloskot Exp $
# 
# Copyright (C) 2006, Mateusz Loskot
#
# Author: Mateusz Loskot <mateusz@loskot.net>
#
AM_CPPFLAGS = $(BOOST_CPPFLAGS)

socidir = $(includedir)/soci

noinst_HEADERS = \
	common-tests.h

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
