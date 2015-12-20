CC=g++
CXXFLAGS=-g -Wall -pedantic -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -Wno-format -Wno-long-long -I.
LDFLAGS=

ifeq ($(shell uname), Linux)
	CXXFLAGS+=-std=c++11

	CXXFLAGS+=-DHAVE_TCP_CORK -DHAVE_ACCEPT4 -DUSE_FIONBIO -DHAVE_EPOLL -DHAVE_POLL
	CXXFLAGS+=-DHAVE_SENDFILE -DHAVE_MMAP -DHAVE_PREAD -DHAVE_PWRITE
	CXXFLAGS+=-DHAVE_TIMEGM -DHAVE_TIMEZONE
	CXXFLAGS+=-DHAVE_POLLRDHUP -DHAVE_SSL

	LIBS=-lssl
else
	ifeq ($(shell uname), FreeBSD)
		CC=g++49
		CXXFLAGS+=-std=c++11

		CXXFLAGS+=-DHAVE_TCP_NOPUSH -DHAVE_ACCEPT4 -DUSE_FIONBIO -DHAVE_KQUEUE -DHAVE_POLL
		CXXFLAGS+=-DHAVE_SENDFILE -DHAVE_MMAP -DHAVE_PREAD -DHAVE_PWRITE
		CXXFLAGS+=-DHAVE_TIMEGM
		CXXFLAGS+=-DHAVE_SSL

		LIBS=-lssl -lcrypto
	else
		ifeq ($(shell uname), SunOS)
			CXXFLAGS+=-std=c++0x

			CXXFLAGS+=-DHAVE_TCP_CORK -DHAVE_ACCEPT4 -DHAVE_PORT -DHAVE_POLL
			CXXFLAGS+=-DHAVE_SENDFILE -DHAVE_MMAP -DHAVE_PREAD -DHAVE_PWRITE
			CXXFLAGS+=-DHAVE_TIMEZONE
			CXXFLAGS+=-DHAVE_SSL

			LIBS=-lssl -lcrypto -lsocket -lsendfile -lnsl
		else
			ifeq ($(shell uname), Minix)
				CC=clang++
				CXXFLAGS+=-std=c++11

				CXXFLAGS+=-I/usr/pkg/include
				CXXFLAGS+=-DHAVE_POLL
				CXXFLAGS+=-DHAVE_MMAP -DHAVE_PREAD -DHAVE_PWRITE
				CXXFLAGS+=-DHAVE_TIMEGM
				CXXFLAGS+=-DHAVE_SSL

				LDFLAGS+=-L/usr/pkg/lib
				LIBS=-lssl -lcrypto
			endif
		endif
	endif
endif

MAKEDEPEND=${CC} -MM
PROGRAM=downloader

OBJS =	constants/months_and_days.o \
	string/buffer.o fs/file.o \
	util/ranges.o util/number.o \
	net/http/date.o \
	net/http/header/permanent_header.o net/http/header/non_permanent_header.o \
	net/http/header/headers.o net/socket_address.o net/ipv4_address.o \
	net/ipv6_address.o net/ports.o \
	net/socket.o net/fdmap.o net/tcp_connection.o net/filesender.o \
	net/uri/uri.o \
	net/http/methods.o net/http/client.o net/http/downloader.o \
	main.o

ifneq ($(filter -DHAVE_EPOLL, $(CXXFLAGS)),)
	OBJS+=net/epoll_selector.o
else
	ifneq ($(filter -DHAVE_KQUEUE, $(CXXFLAGS)),)
		OBJS+=net/kqueue_selector.o
	else
		ifneq ($(filter -DHAVE_PORT, $(CXXFLAGS)),)
			OBJS+=net/port_selector.o
		else
			ifneq ($(filter -DHAVE_POLL, $(CXXFLAGS)),)
				OBJS+=net/poll_selector.o
			else
				OBJS+=net/select_selector.o
			endif
		endif
	endif
endif

ifneq (,$(findstring HAVE_SSL, $(CXXFLAGS)))
	OBJS+=net/ssl_socket.o
endif

DEPS:= ${OBJS:%.o=%.d}

all: $(PROGRAM)

${PROGRAM}: ${OBJS}
	${CC} ${LDFLAGS} ${OBJS} ${LIBS} -o $@

clean:
	rm -f ${PROGRAM} ${OBJS} ${DEPS}

${OBJS} ${DEPS} ${PROGRAM} : Makefile

.PHONY : all clean

%.d : %.cpp
	${MAKEDEPEND} ${CXXFLAGS} $< -MT ${@:%.d=%.o} > $@

%.o : %.cpp
	${CC} ${CXXFLAGS} -c -o $@ $<

-include ${DEPS}
