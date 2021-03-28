export CC      ?= $(CROSS_COMPILE)gcc
export STRIP   ?= $(CROSS_COMPILE)strip
export CFLAGS  += -O3 -g
export LDFLAGS +=
LIBS+= -lpthread
BINS = adbd
all: all-bin
clean-adbd:
	$(MAKE) -C src/adbd clean
clean-libcutils:
	$(MAKE) -C src/libcutils clean
clean: clean-adbd clean-libcutils
	rm -f adbd adbd_debug
all-bin: $(BINS)
FORCE:
.PHONY: all all-bin clean clean-adbd clean-libcutils FORCE
src/adbd/adbd.a: src/adbd/Makefile FORCE
	$(MAKE) -C src/adbd adbd.a
src/libcutils/libcutils.a: src/libcutils/Makefile FORCE
	$(MAKE) -C src/libcutils libcutils.a
adbd_debug: src/main.c src/adbd/adbd.a src/libcutils/libcutils.a
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
adbd: adbd_debug
	$(STRIP) $< -o $@
install: adbd configs/adbd configs/adbd.service
	install -Dm0644 configs/adbd.service $(DESTDIR)/usr/lib/systemd/system/adbd.service
	install -Dm0644 configs/adbd $(DESTDIR)/etc/default/adbd
	install -Dm0755 adbd $(DESTDIR)/usr/bin/adbd
