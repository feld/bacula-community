#
# Simple Makefile for building test FD plugins for Bacula
#
# Copyright (C) 2000-2015 by Kern Sibbald
# License: BSD 2-Clause; see file LICENSE-FOSS
#
#

include ../../Makefile.inc

LDAPSRC = ldap-dir.c
LDAPTESTSRC = ldaptest.c
LDAPOBJ = $(LDAPSRC:.c=.lo)
LDAPTESTOBJ = $(LDAPTESTSRC:.c=.lo)

# ldap-dir.la ldaptest
all: $(BPAM_LDAP_TARGET)

.c.lo:
	@echo "Compiling $< ..."
	$(NO_ECHO)$(LIBTOOL_COMPILE) $(CXX) $(DEFS) $(DEBUG) $(CPPFLAGS) -I${SRCDIR} -I${DIRDIR} -I${DIRPLUGDIR} $(LDAP_INC) -c $<

%.lo: %.c %.h Makefile
	@echo "Compiling $< ..."
	$(NO_ECHO)$(LIBTOOL_COMPILE) $(CXX) $(DEFS) $(DEBUG) $(CPPFLAGS) $(CFLAGS) -I${SRCDIR} -I${DIRDIR} -I${DIRPLUGDIR} $(LDAP_INC) -c $(@:.lo=.c)

ldap-dir.la: Makefile $(LDAPOBJ) $(COMMONOBJ) $(DIRPLUGDIR)/dirpluglib.lo
	@echo "Linking $(@:.la=.so) ..."
	$(NO_ECHO)$(LIBTOOL_LINK) --silent $(CXX) $(LDFLAGS) $(LDAP_LDFLAGS) $(LDAP_LIBS) -shared $^ -o $@ -rpath $(plugindir) -module -export-dynamic -avoid-version

$(DIRPLUGDIR)/dirpluglib.lo:
	$(MAKE) -C $(DIRPLUGDIR) dirpluglib.lo

ldaptest: $(LDAPTESTOBJ) Makefile
	@echo "Building ldaptest ..."
	$(NO_ECHO)$(LIBTOOL_LINK) --silent $(CXX) $(LDFLAGS) $(LDAP_LDFLAGS) $(LDAP_LIBS) ldaptest.lo -o $@

install: $(BPAM_LDAP_TARGET_INSTALL)

install-ldap: ldap-dir.la
	@echo "Installing plugin $(^:.la=.so) ..."
	$(MKDIR) $(DESTDIR)$(plugindir)
	$(LIBTOOL_INSTALL) $(INSTALL_PROGRAM) ldap-dir.la $(DESTDIR)$(plugindir)
	$(NO_ECHO)$(RMF) $(DESTDIR)$(plugindir)/ldap-dir.la

install-ldaptest: ldaptest
	@echo "Installing $< ..."
	$(LIBTOOL_INSTALL) $(INSTALL_PROGRAM) $< $(DESTDIR)$(sbindir)

Makefile: Makefile.in $(topdir)/config.status
	cd $(topdir) \
	  && CONFIG_FILES=$(thisdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

$(LIBTOOL_CLEAN_TARGET):
	@find . -name '*.lo' -print | xargs $(LIBTOOL_CLEAN) $(RMF)
	@$(RMF) *.la
	@$(RMF) -r .libs _libs

clean: $(LIBTOOL_CLEAN_TARGET)
	@rm -f main ldaptest *.so *.o

distclean: clean
	@rm -f Makefile *.la *.lo
	@rm -rf .libs

$(LIBTOOL_UNINSTALL_TARGET):
	$(LIBTOOL_UNINSTALL) $(RMF) $(DESTDIR)$(plugindir)/ldap-dir.so
	$(LIBTOOL_UNINSTALL) $(RMF) $(DESTDIR)$(plugindir)/msad-fd.so

uninstall: $(LIBTOOL_UNINSTALL_TARGET)

depend: