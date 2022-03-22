#
# Makefile for building FD plugins PluginLibrary for Bacula
#
# Copyright (C) 2000-2020 Kern Sibbald
# License: BSD 2-Clause; see file LICENSE-FOSS
#
#  Author: Radoslaw Korzeniewski, radoslaw@korzeniewski.net
#

include ../Makefile.inc

thisdir = $(FDPLUGDIR)/docker

UNITTESTSOBJ = $(LIBDIR)/unittests.lo
LIBBACOBJ = $(LIBDIR)/libbac.la

DOCKERSRC = dkid.c dkinfo.c dkcommctx.c docker-fd.c
DOCKERSRCH = dkid.h dkinfo.h dkcommctx.h docker-fd.h
DOCKEROBJ = $(DOCKERSRC:.c=.lo)
DOCKERTESTS = dkid_test

all: docker-fd.la

tests: $(DOCKERTESTS)

.c.lo:
	@echo "Compiling $< ..."
	$(NO_ECHO)$(LIBTOOL_COMPILE) $(CXX) $(DEFS) $(DEBUG) $(CPPFLAGS) $(CFLAGS) -I${SRCDIR} -I${FDDIR} -I${FDPLUGDIR} -I${LIBDIR} -DWORKDIR=\"$(DESTDIR)$(working_dir)\" -c $<

%.lo: %.c %.h
	@echo "Pattern compiling $< ..."
	$(NO_ECHO)$(LIBTOOL_COMPILE) $(CXX) $(DEFS) $(DEBUG) $(CPPFLAGS) $(CFLAGS) -I${SRCDIR} -I${FDDIR} -I${FDPLUGDIR} -I${LIBDIR} -DWORKDIR=\"$(DESTDIR)$(working_dir)\" -c $(@:.lo=.c)

docker-fd.la: Makefile $(DOCKEROBJ) $(PLUGINLIBDIR)/pluginlib.lo $(DOCKERSRCH)
	@echo "Linking $(@:.la=.so) ..."
	$(NO_ECHO)$(LIBTOOL_LINK) --silent $(CXX) $(LDFLAGS) -shared $^ -o $@ -rpath $(plugindir) -module -export-dynamic -avoid-version

dkid_test: Makefile dkid.lo dkid_test.lo $(UNITTESTSOBJ) $(LIBBACOBJ)
	@echo "Building $@ ..."
	$(NO_ECHO)$(LIBTOOL_LINK) $(CXX) $(LDFLAGS) -L$(LIBDIR) -o $@ dkid.lo dkid_test.lo $(UNITTESTSOBJ) $(DLIB) -lbac -lm $(LIBS) $(OPENSSL_LIBS)

install: all install-docker

install-docker: docker-fd.la
	@echo "Installing plugin $(^:.la=.so) ..."
	$(MKDIR) $(DESTDIR)$(plugindir)
	$(LIBTOOL_INSTALL) $(INSTALL_PROGRAM) docker-fd.la $(DESTDIR)$(plugindir)
	$(NO_ECHO)$(RMF) $(DESTDIR)$(plugindir)/docker-fd.la

libtool-clean:
	@find . -name '*.lo' -print | xargs $(LIBTOOL_CLEAN) $(RMF)
	@find . -name '*.la' -print | xargs $(LIBTOOL_CLEAN) $(RMF)
	@$(RMF) -r .libs _libs

clean: libtool-clean
	@$(RMF) -f main *.so *.o
	@$(RMF) -f $(DOCKERTESTS)

distclean: clean

libtool-uninstall:
	$(LIBTOOL_UNINSTALL) $(RMF) $(DESTDIR)$(plugindir)/docker-fd.so

depend:

uninstall: $(LIBTOOL_UNINSTALL_TARGET)
