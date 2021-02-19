#
# Simple Makefile for building test FD plugins for Bacula
#
# Copyright (C) 2000-2015 Kern Sibbald
# License: BSD 2-Clause; see file LICENSE-FOSS
#

include Makefile.inc

.c.lo:
	$(LIBTOOL_COMPILE) $(CXX) $(DEFS) $(DEBUG) $(CPPFLAGS) $(CFLAGS) -I${SRCDIR} -I${DIRDIR} -DTEST_PROGRAM -c $<

all: dirpluglib.lo

example-plugin-dir.lo: example-plugin-dir.c ${DIRDIR}/dir_plugins.h
	$(LIBTOOL_COMPILE) $(CXX) $(DEFS) $(DEBUG) $(CFLAGS) -I../.. -I${DIRDIR} -c example-plugin-dir.c

example-plugin-dir.la: Makefile example-plugin-dir.lo
	$(LIBTOOL_LINK) $(CXX) $(LDFLAGS) -shared example-plugin-dir.lo -o $@ -rpath $(plugindir) -module -export-dynamic -avoid-version

test-authentication-api-dir.lo: test-authentication-api-dir.c ${DIRDIR}/dir_plugins.h
	$(LIBTOOL_COMPILE) $(CXX) $(DEFS) $(DEBUG) $(CFLAGS) -I../.. -I${DIRDIR} -c test-authentication-api-dir.c

test-authentication-api-dir.la: Makefile test-authentication-api-dir.lo dirpluglib.lo
	@echo "Linking $(@:.la=.so) ..."
	$(NO_ECHO)$(LIBTOOL_LINK) --silent $(CXX) $(LDFLAGS) -shared $^ -o $@ -rpath $(plugindir) -module -export-dynamic -avoid-version

install: all
	$(MKDIR) $(DESTDIR)$(plugindir)

$(LIBTOOL_CLEAN_TARGET):
	find . -name '*.lo' -print | xargs $(LIBTOOL_CLEAN) $(RMF)
	$(RMF) *.la
	$(RMF) -r .libs _libs

clean: $(LIBTOOL_CLEAN_TARGET)
	rm -f main *.so *.o 1 2 3

distclean: clean
	rm -f Makefile

$(LIBTOOL_UNINSTALL_TARGET):
	$(LIBTOOL_UNINSTALL) $(RMF) $(DESTDIR)$(plugindir)/example-plugin-dir.la

uninstall: $(LIBTOOL_UNINSTALL_TARGET)

depend:
