SRCS+=adb.c
SRCS+=adb_auth_client.c
SRCS+=fdevent.c
SRCS+=transport.c
SRCS+=transport_local.c
SRCS+=transport_usb.c
SRCS+=sockets.c
SRCS+=services.c
SRCS+=file_sync_service.c
SRCS+=jdwp_service.c
SRCS+=framebuffer_service.c
SRCS+=remount_service.c
SRCS+=usb_linux_client.c
SRCS+=log_service.c
SRCS+=base64.c
CPPFLAGS+=-Wno-pointer-to-int-cast -Wno-unused-parameter -Wno-int-to-pointer-cast -Wall -Wextra
CPPFLAGS+=-I. -I../include
OBJS=$(SRCS:.c=.o)
all: adbd.a
adbd.a: $(OBJS)
	$(AR) cr adbd.a $(OBJS)
clean:
	rm -f *.o *.a *.plist
analyze:
	clang $(CPPFLAGS) --analyze $(SRCS)
.PHONY: clean all analyze
