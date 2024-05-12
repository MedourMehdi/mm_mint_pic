_CC = m68k-atari-mint-g++
SRC_DIR := ./
OBJ_DIR := ./build
BIN_DIR := ./bin

DEFINES :=
WITH_WAVLIB := NO
WITH_FFMPEG := YES
WITH_FFMPEG_SOUND := YES
WITH_PSD := YES
WITH_XPDF := YES

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

BIN := $(BIN_DIR)/mm_pic.prg

OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

_CPPFLAGS := -I./ -I/opt/cross-mint/m68k-atari-mint/include/freetype2

_CFLAGS   := -m68020-60 -fomit-frame-pointer -fno-strict-aliasing -O2 $(DEFINES)

_LDFLAGS  :=

_LDLIBS   := -lgem -lpng -lz -lyuv -lheif -lwebp -lwebpdemux -ljpeg -ltiff -llzma -lde265 -lx265 -lpthread -lgif

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

$(BIN): $(OBJ) | $(BIN_DIR)
	$(_CC) $(_LDFLAGS) $^ $(_LDLIBS) -o $@
	m68k-atari-mint-strip $(BIN)
	
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(@D)
	$(_CC) $(_CPPFLAGS) $(_CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $(@D)

clean:
	@$(RM) -rv $(BIN) $(OBJ_DIR)

clean-objects:
	@$(RM) -rv $(OBJ_DIR)

-include $(OBJ:.o=.d)