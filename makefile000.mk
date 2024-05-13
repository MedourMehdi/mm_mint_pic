_CXX = m68k-atari-mint-g++
_CC = m68k-atari-mint-gcc
SRC_DIR := ./
OBJ_DIR := ./build
OBJ_DIR_C := ./build
BIN_DIR := ./bin

DEFINES :=
WITH_WAVLIB := YES
WITH_FFMPEG := NO
WITH_FFMPEG_SOUND := NO
WITH_PSD := YES
WITH_XPDF := YES
WITH_RECOIL := YES

ifeq ($(WITH_RECOIL), YES)
DEFINES += -DWITH_RECOIL=1
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

LIB_FFMPEG := -lavformat -lavcodec -lavutil -lswscale -lswresample -lfribidi -llcms2 -lxml2 -liconv -lssl -lcrypto -lfreetype -lbz2 -lpng16 -lm -lz -lpthread -lwebp -lvpx -llzma -lx264 -lx265 -lstdc++ -ltheora -lopus -lwebpdemux -lwebpmux -lwebpdecoder -lvorbisenc -lvorbis -logg -lmp3lame -laacplus -laom -lfdk-aac
LIB_XPDF := -lxpdf -lfofi -lgoo -lsplash 
LIB_FREETYPE := -lfreetype -lbz2
LIB_PSD := -lpsd_malloc 

SRC := $(wildcard $(SRC_DIR)/*.cpp) \
  $(wildcard $(SRC_DIR)/*/*.cpp) \
  $(wildcard $(SRC_DIR)/*/zview/*.cpp) \
  $(wildcard $(SRC_DIR)/*/dither/*.cpp) \
  $(wildcard $(SRC_DIR)/*/nanosvg/*.cpp) \
  $(wildcard $(SRC_DIR)/*/qdbmp/*.cpp) \
  $(wildcard $(SRC_DIR)/*/rgb2lab/*.cpp) \
  $(wildcard $(SRC_DIR)/*/tgafunc/*.cpp) \
  $(wildcard $(SRC_DIR)/*/flic/*.cpp) 

ifeq ($(WITH_WAVLIB), YES)
SRC += $(wildcard $(SRC_DIR)/*/wav_lib/*.cpp)
endif

ifeq ($(WITH_RECOIL), YES)
SRC_C := $(wildcard $(SRC_DIR)/*/recoil/*.c)
endif

BIN := $(BIN_DIR)/mm_pic.prg

OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
OBJ_C := $(SRC_C:$(SRC_DIR)/%.c=$(OBJ_DIR_C)/%.o)

_CPPFLAGS := -I./ -I/opt/cross-mint/m68k-atari-mint/include/freetype2

_CFLAGS   := -m68000 -fomit-frame-pointer -fno-strict-aliasing -O2 $(DEFINES)

_LDFLAGS  :=

_LDLIBS   := -lgem -lpng -lz -lyuv -lheif -lwebp -lwebpdemux -ljpeg -ltiff -lzstd -lwebp -llzma -lde265 -lx265 -lpthread -lgif

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


# _CFLAGS += -Wl,--stack,10485760

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJ) $(OBJ_C) | $(BIN_DIR)
	$(_CXX) $(_LDFLAGS) $^ $(_LDLIBS) -o $@
	m68k-atari-mint-strip $(BIN)
	
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