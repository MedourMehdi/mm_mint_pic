_CC = m68k-atari-mint-g++
SRC_DIR := ./
OBJ_DIR := ./build
BIN_DIR := ./bin

DEFINES :=
WITH_WAVLIB := NO
WITH_FFMPEG := YES
WITH_FFMPEG_SOUND := YES


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

BIN := $(BIN_DIR)/mm_piccf.prg

OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

_CPPFLAGS := -I./

_CFLAGS   := -mcfv4e -fomit-frame-pointer -fno-strict-aliasing -O2 $(DEFINES)

_LDFLAGS  :=

_LDLIBS   := -lgem -lpng -lz -lyuv -lheif -lwebp -lwebpdemux -ljpeg -ltiff -lzstd -llzma -lde265 -lx265 -lpthread -lgif $(LIB_XPDF) $(LIB_FREETYPE) -lpsd_malloc

ifeq ($(WITH_FFMPEG), YES)
_LDLIBS += $(LIB_FFMPEG)
endif

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