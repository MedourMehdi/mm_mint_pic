_CXX = m68k-atari-mint-g++
_CC = m68k-atari-mint-gcc
_AS = vasmm68k_mot
_ASFLAGS = -m68020 -Faout -quiet

# STACK_SIZE := 524288
STACK_SIZE := 262144

SRC_DIR := ./
OBJ_DIR := ./build
OBJ_DIR_C := ./build
OBJ_DIR_S := ./build
BIN_DIR := ./bin

DEFINES :=
WITH_WAVLIB := NO
WITH_FFMPEG := NO
WITH_FFMPEG_SOUND := NO
# WITH_FFMPEG := YES
# WITH_FFMPEG_SOUND := YES
WITH_PSD := YES
WITH_XPDF := YES
WITH_RECOIL := YES
WITH_XBRZ := YES
WITH_VASM := NO
WITH_CURL := NO

ifeq ($(WITH_XBRZ), YES)
DEFINES += -DWITH_XBRZ=1
endif

ifeq ($(WITH_RECOIL), YES)
DEFINES += -DWITH_RECOIL=1
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

ifeq ($(WITH_FFMPEG), YES)
DEFINES += -DWITH_FFMPEG=1
ifeq ($(WITH_FFMPEG_SOUND), YES)
DEFINES += -DWITH_FFMPEG_SOUND=1
endif
endif

DEFINES += -DSTACK_SIZE=$(STACK_SIZE)

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
  $(wildcard $(SRC_DIR)/*/nanosvg/*.cpp) \
  $(wildcard $(SRC_DIR)/*/qdbmp/*.cpp) \
  $(wildcard $(SRC_DIR)/*/rgb2lab/*.cpp) \
  $(wildcard $(SRC_DIR)/*/tgafunc/*.cpp) \
  $(wildcard $(SRC_DIR)/*/flic/*.cpp) 

ifeq ($(WITH_VASM), YES)
SRC_S := $(wildcard $(SRC_DIR)/*/asm/*.s)
endif

ifeq ($(WITH_WAVLIB), YES)
SRC += $(wildcard $(SRC_DIR)/*/wav_lib/*.cpp)
endif

ifeq ($(WITH_RECOIL), YES)
SRC_C := $(wildcard $(SRC_DIR)/*/recoil/*.c)
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

_CFLAGS   := -m68020-60 -fomit-frame-pointer -fno-strict-aliasing -O2 $(DEFINES)

_LDFLAGS  :=

_LDLIBS   := -lgem -lpng -lz -lyuv -lheif -lwebp -lwebpdemux -ljpeg $(LIB_TIFF) -lde265 -lx265 -lpthread -lgif

ifeq ($(WITH_XPDF), YES)
_LDLIBS += $(LIB_XPDF)
endif

_LDLIBS += $(LIB_FREETYPE)

ifeq ($(WITH_PSD), YES)
_LDLIBS += $(LIB_PSD)
endif

ifeq ($(WITH_FFMPEG), YES)
_LDLIBS += $(LIB_FFMPEG)
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