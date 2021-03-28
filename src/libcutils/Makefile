SRCS+=socket_inaddr_any_server.c
SRCS+=socket_local_client.c
SRCS+=socket_local_server.c
SRCS+=socket_loopback_client.c
SRCS+=socket_loopback_server.c
SRCS+=list.c
CPPFLAGS+=-I../include
OBJS=$(patsubst %, %.o, $(basename $(SRCS)))
all: libcutils.a
libcutils.a: $(OBJS)
	$(AR) cr $@ $(OBJS)
clean:
	rm -f *.o *.a
.PHONY: clean all