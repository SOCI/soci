# $Id: Makefile.am,v 1.5 2006/12/09 15:34:15 mloskot Exp $
# 
# Copyright (C) 2006, Mateusz Loskot
#
# Author: Mateusz Loskot <mateusz@loskot.net>
#
ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = \
	1.4 \
	foreign \
	dist-bzip2 \
	dist-zip

SUBDIRS = \
    core \
    backends

# Commands
RM = rm -rf
COPY = cp -p

# Clean patterns
CLEANFILES = *~

MAINTAINERCLEANFILES = \
	aclocal.m4 \
	depcomp \
	config.cache \
	config.guess \
	config.h.in \
	config.log \
	config.status \
	config.sub \
	configure \
	install-sh \
	libtool \
	ltmain.sh \
	missing \
	stamp-h.in

EXTRA_DIST = \
	autogen.sh \
	../build \
	../doc

DISTCLEANLOCAL = \
	autom4te.cache

MAINTAINERCLEANLOCAL = Makefile.in

dist-hook:
	$(COPY) $(top_srcdir)/contrib $(distdir)
	$(COPY) $(top_srcdir)/CHANGES $(distdir)
	$(COPY) $(top_srcdir)/README $(distdir)
	$(COPY) $(top_srcdir)/LICENSE_1_0.txt $(distdir)

distclean-local:
	-test -z "$(DISTCLEANLOCAL)" || rm -rf $(DISTCLEANLOCAL)

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)

