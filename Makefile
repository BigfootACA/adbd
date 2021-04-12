export CC      ?= $(CROSS_COMPILE)gcc
export STRIP   ?= $(CROSS_COMPILE)strip
export CFLAGS  += -O3 -g
export LDFLAGS +=
PREFIX?=/usr
BINDIR?=$(PREFIX)/bin
LIBDIR?=$(PREFIX)/lib
SYSCONFDIR?=/etc
MISCCONFDIR?=$(SYSCONFDIR)/default
SYSTEMDDIR?=$(LIBDIR)/systemd
SYSTEMDUNITDIR?=$(SYSTEMDDIR)/system
LIBS+= -lpthread
BINS = adbd
all: all-bin
clean-adbd:
	$(MAKE) -C src/adbd clean
clean-libcutils:
	$(MAKE) -C src/libcutils clean
clean-bins:
	rm -f adbd adbd_debug
clean: clean-bins clean-adbd clean-libcutils
analyze-adbd:
	$(MAKE) -C src/adbd analyze
analyze-libcutils:
	$(MAKE) -C src/libcutils analyze
analyze: analyze-adbd analyze-libcutils
all-bin: $(BINS)
FORCE:
.PHONY: all all-bin clean clean-bins clean-adbd clean-libcutils FORCE install uninstall analyze analyze-adbd analyze-libcutils
src/adbd/adbd.a: src/adbd/Makefile FORCE
	$(MAKE) -C src/adbd adbd.a
src/libcutils/libcutils.a: src/libcutils/Makefile FORCE
	$(MAKE) -C src/libcutils libcutils.a
adbd_debug: src/main.c src/adbd/adbd.a src/libcutils/libcutils.a
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
adbd: adbd_debug
	$(STRIP) $< -o $@
configs/adbd.service: configs/adbd.service.in
	sed \
		-e 's@%PREFIX%@$(PREFIX)@g' \
		-e 's@%BINDIR%@$(BINDIR)@g' \
		-e 's@%LIBDIR%@$(LIBDIR)@g' \
		-e 's@%SYSCONFDIR%@$(SYSCONFDIR)@g' \
		-e 's@%MISCCONFDIR%@$(MISCCONFDIR)@g' \
		-e 's@%SYSTEMDDIR%@$(SYSTEMDDIR)@g' \
		-e 's@%SYSTEMDUNITDIR%@$(SYSTEMDUNITDIR)@g' \
	$< > $@
install: adbd configs/adbd.conf configs/adbd.service
	install -Dm0644 configs/adbd.service $(DESTDIR)$(SYSTEMDUNITDIR)/adbd.service
	install -Dm0644 configs/adbd.conf $(DESTDIR)$(MISCCONFDIR)/adbd
	install -Dm0755 adbd $(DESTDIR)$(BINDIR)/adbd
uninstall:
	rm -fv $(DESTDIR)$(SYSTEMDUNITDIR)/adbd.service
	rm -fv $(DESTDIR)$(MISCCONFDIR)/adbd
	rm -fv $(DESTDIR)$(BINDIR)/adbd
