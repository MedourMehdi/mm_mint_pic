PREFIX = /opt/cross-mint/m68k-atari-mint
CC = m68k-atari-mint-gcc
AR = m68k-atari-mint-ar
RANLIB = m68k-atari-mint-ranlib
ARFLAGS = -rcs
CFLAGS = -m68020-60 -fomit-frame-pointer -fno-strict-aliasing -O2

C_LIB_SOURCES = qdbmp.c
C_LIB_OBJECTS = qdbmp.o
C_TARGETS = libqdbmp.a

ALL_TARGETS = $(C_TARGETS)

lib: libqdbmp.a

install: libqdbmp.a
	install libqdbmp.a $(PREFIX)/lib/

uninstall:
	rm -f $(PREFIX)/lib/libqdbmp.a

$(C_TARGETS): $(C_LIB_OBJECTS) $(C_LIB_SOURCES:.c=.h)

all: $(ALL_TARGETS)

$(C_LIB_OBJECTS):
	# $(CC) $(CFLAGS) $(C_LIB_SOURCES) -o $@
	$(CC) $(CFLAGS) -c $(C_LIB_SOURCES)

libqdbmp.a:
	$(AR) $(ARFLAGS) $@ $(C_LIB_OBJECTS)

clean:
	rm -f $(ALL_TARGETS) $(C_LIB_OBJECTS)