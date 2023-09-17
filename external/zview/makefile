PREFIX = /opt/cross-mint/m68k-atari-mint
CC = m68k-atari-mint-gcc
AR = m68k-atari-mint-ar
RANLIB = m68k-atari-mint-ranlib
ARFLAGS = -rcs
CFLAGS = -m68020-60 -fomit-frame-pointer -fno-strict-aliasing -O2

C_LIB_SOURCES = zview_color.c
C_LIB_OBJECTS = zview_color.o
C_TARGETS = libzview_color.a

ALL_TARGETS = $(C_TARGETS)

lib: libzview_color.a

install: libzview_color.a
	install libzview_color.a $(PREFIX)/lib/

uninstall:
	rm -f $(PREFIX)/lib/libzview_color.a

$(C_TARGETS): $(C_LIB_OBJECTS) $(C_LIB_SOURCES:.c=.h)

all: $(ALL_TARGETS)

$(C_LIB_OBJECTS):
	# $(CC) $(CFLAGS) $(C_LIB_SOURCES) -o $@
	$(CC) $(CFLAGS) -c $(C_LIB_SOURCES)

libzview_color.a:
	$(AR) $(ARFLAGS) $@ $(C_LIB_OBJECTS)

clean:
	rm -f $(ALL_TARGETS) $(C_LIB_OBJECTS)