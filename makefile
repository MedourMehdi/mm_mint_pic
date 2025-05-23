_CXX = m68k-atari-mint-g++
_CC = m68k-atari-mint-gcc
_AS = vasmm68k_mot
_ASFLAGS = -m68020 -Faout -quiet

# recoil recommends at least 256kb for stack size
# STACK_SIZE := 524288
STACK_SIZE := 262144

SRC_DIR := ./
OBJ_DIR := ./build
OBJ_DIR_C := ./build
OBJ_DIR_S := ./build
BIN_DIR := ./bin

DEFINES :=

# Sound libraries
WITH_WAVLIB := YES
WITH_MP3LIB := YES
# WITH_WAVLIB := NO
# WITH_MP3LIB := NO

WITH_FFMPEG := NO
WITH_FFMPEG_SOUND := NO
# WITH_FFMPEG := YES
# WITH_FFMPEG_SOUND := YES

WITH_PSD := YES
# WITH_PSD := NO

WITH_TIFF := YES
# WITH_TIFF := NO

WITH_XPDF := YES
# WITH_XPDF := NO

WITH_RECOIL := YES
WITH_XBRZ := YES

# This will not work if used with Coldfire
# For now only 4bpp C2P can use ASM
WITH_VASM := NO

# Not yet finished. Purpose is to handle URL files
WITH_CURL := NO
WITH_URL := NO

# Cached icons for screens <= 8bpp
WITH_CACHE := YES

# SVG libraries, choose only one!
WITH_NANOSVG := NO
WITH_LUNASVG := YES
# WITH_NANOSVG := YES
# WITH_LUNASVG := NO

WITH_FREETYPE := NO

ifeq ($(WITH_XBRZ), YES)
DEFINES += -DWITH_XBRZ=1
endif

ifeq ($(WITH_FREETYPE), YES)
DEFINES += -WITH_FREETYPE=1
endif

ifeq ($(WITH_TIFF), YES)
DEFINES += -DWITH_TIFF=1
endif

ifeq ($(WITH_RECOIL), YES)
DEFINES += -DWITH_RECOIL=1
# DEFINES += -DUSE_CUSTOM_RECOIL_CHECK=1
endif

ifeq ($(WITH_CACHE), YES)
DEFINES += -DWITH_CACHE=1
endif

ifeq ($(WITH_CURL), YES)
DEFINES += -DWITH_CURL=1
endif

ifeq ($(WITH_VASM), YES)
DEFINES += -DWITH_VASM=1
endif

ifeq ($(WITH_XPDF), YES)
DEFINES += -DWITH_XPDF=1
endif

ifeq ($(WITH_PSD), YES)
DEFINES += -DWITH_PSD=1
endif

ifeq ($(WITH_WAVLIB), YES)
DEFINES += -DWITH_WAVLIB=1
endif

ifeq ($(WITH_MP3LIB), YES)
DEFINES += -DWITH_MP3LIB=1
endif

ifeq ($(WITH_NANOSVG), YES)
DEFINES += -DWITH_NANOSVG=1
endif

ifeq ($(WITH_LUNASVG), YES)
DEFINES += -DWITH_LUNASVG=1
endif


ifeq ($(WITH_FFMPEG), YES)
DEFINES += -DWITH_FFMPEG=1 -DUSE_LNX_PATH=1
ifeq ($(WITH_FFMPEG_SOUND), YES)
DEFINES += -DWITH_FFMPEG_SOUND=1 -DUSE_LNX_PATH=1
endif
endif

DEFINES += -DSTACK_SIZE=$(STACK_SIZE)

LIB_PLUTOSVG := -llunasvg -lplutovg
LIB_SSL := -lssl -lcrypto -lz
LIB_FFMPEG := -lavformat -lavcodec -lavutil -lswscale -lswresample -lfribidi -llcms2 -lxml2 -liconv -lssl -lcrypto -lfreetype -lbz2 -lpng16 -lm -lz -lpthread -lwebp -lvpx -llzma -lx264 -lx265 -lstdc++ -ltheora -lopus -lwebpdemux -lwebpmux -lwebpdecoder -lvorbisenc -lvorbis -logg -lmp3lame -laacplus -laom -lfdk-aac
LIB_XPDF := -lxpdf -lfofi -lgoo -lsplash 
LIB_FREETYPE := -lfreetype -lbz2
LIB_PSD := -lpsd_malloc 
# LIB_TIFF :=  -ljpeg -lz -lm -ltiff
LIB_TIFF := -ltiff -lwebp -lzstd -llzma -ljpeg -lz -lm
LIB_CURL := -lcurl -lnghttp2 -lidn2 -liconv -lssh2 -lpsl -lunistring -liconv $(LIB_SSL)

SRC := $(wildcard $(SRC_DIR)/*.cpp) \
  $(wildcard $(SRC_DIR)/*/*.cpp) \
  $(wildcard $(SRC_DIR)/*/zview/*.cpp) \
  $(wildcard $(SRC_DIR)/*/dither/*.cpp) \
  $(wildcard $(SRC_DIR)/*/qdbmp/*.cpp) \
  $(wildcard $(SRC_DIR)/*/rgb2lab/*.cpp) \
  $(wildcard $(SRC_DIR)/*/tgafunc/*.cpp) \
  $(wildcard $(SRC_DIR)/*/md5/*.cpp) \
  $(wildcard $(SRC_DIR)/*/flic/*.cpp) 

ifeq ($(WITH_VASM), YES)
SRC_S := $(wildcard $(SRC_DIR)/*/asm/*.s)
endif

ifeq ($(WITH_NANOSVG), YES)
SRC += $(wildcard $(SRC_DIR)/*/nanosvg/*.cpp)
endif

ifeq ($(WITH_WAVLIB), YES)
SRC += $(wildcard $(SRC_DIR)/*/wav_lib/*.cpp)
endif

ifeq ($(WITH_MP3LIB), YES)
SRC += $(wildcard $(SRC_DIR)/*/minimp3/*.cpp)
endif

ifeq ($(WITH_RECOIL), YES)
SRC_C := $(wildcard $(SRC_DIR)/*/recoil/*.c)
endif

ifeq ($(WITH_CACHE), YES)
SRC_C += $(wildcard $(SRC_DIR)/*/md5/*.c)
endif

ifeq ($(WITH_XBRZ), YES)
SRC +=   $(wildcard $(SRC_DIR)/*/xbrz/*.cpp)
endif

BIN := $(BIN_DIR)/mm_pic.prg

OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
OBJ_C := $(SRC_C:$(SRC_DIR)/%.c=$(OBJ_DIR_C)/%.o)
OBJ_S := $(SRC_S:$(SRC_DIR)/%.s=$(OBJ_DIR_S)/%.o)

ALL_OBJ := $(OBJ) $(OBJ_C)

ifeq ($(WITH_VASM), YES)
ALL_OBJ += $(OBJ_S)
endif


_CPPFLAGS := -I./ -I/opt/cross-mint/m68k-atari-mint/include/freetype2

_CPU	:= -m68020-60

# _CPU	:= -m68020 -m68881

_CFLAGS   :=  $(_CPU) -fomit-frame-pointer -fno-strict-aliasing -O3 $(DEFINES)

_LDFLAGS  :=

_LDLIBS   := -lgem -lpng -lz -lyuv -lheif -lwebp -lwebpdemux -ljpeg -lde265 -lx265 -lpthread -lgif

ifeq ($(WITH_TIFF), YES)
_LDLIBS += $(LIB_TIFF)
endif

ifeq ($(WITH_LUNASVG), YES)
_LDLIBS += $(LIB_PLUTOSVG)
endif

ifeq ($(WITH_XPDF), YES)
_LDLIBS += $(LIB_XPDF)
WITH_FREETYPE := YES
DEFINES += -WITH_FREETYPE=1
endif

ifeq ($(WITH_FREETYPE), YES)
_LDLIBS += $(LIB_FREETYPE)
endif

ifeq ($(WITH_PSD), YES)
_LDLIBS += $(LIB_PSD)
endif

ifeq ($(WITH_FFMPEG), YES)
_LDLIBS += $(LIB_FFMPEG)
WITH_FREETYPE := YES
DEFINES += -WITH_FREETYPE=1
endif

ifeq ($(WITH_CURL), YES)
_LDLIBS += $(LIB_CURL)
endif
# _CFLAGS += -Wl,--stack,10485760

.PHONY: all clean

all: $(BIN)

$(BIN): $(ALL_OBJ) | $(BIN_DIR)
	$(_CXX) $(_LDFLAGS) $^ $(_LDLIBS) -o $@
	m68k-atari-mint-strip $(BIN)
	m68k-atari-mint-stack --fix=$(STACK_SIZE) $(BIN)
	m68k-atari-mint-flags -S $(BIN)
	# tar -cvJf $(BIN).tar.xz $(BIN)

ifeq ($(WITH_VASM), YES)
$(OBJ_DIR_S)/%.o: $(SRC_DIR)/%.s | $(OBJ_DIR_S)
	@mkdir -p $(@D)
	$(_AS) $(_ASFLAGS) -o $@ $<
endif

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(@D)
	$(_CXX) $(_CPPFLAGS) $(_CFLAGS) -c $< -o $@

$(OBJ_DIR_C)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR_C)
	@mkdir -p $(@D)
	$(_CC) $(_CPPFLAGS) $(_CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $(@D)

clean:
	@$(RM) -rv $(BIN) $(OBJ_DIR)

clean-objects:
	@$(RM) -rv $(OBJ_DIR)

-include $(OBJ:.o=.d)
-include $(OBJ_C:.o=.d)