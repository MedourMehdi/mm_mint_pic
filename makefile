#CC = m68k-atari-mint-gcc
#CPP = m68k-atari-mint-g++
CC = m68k-atari-mint-g++
SRC_DIR := ./
OBJ_DIR := ./build
BIN_DIR := ./bin


SRC := $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp) $(wildcard $(SRC_DIR)/*/*/*.cpp)

BIN := $(BIN_DIR)/mm_pic.prg

OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CPPFLAGS := -I./

CFLAGS   := -m68020-60 -fomit-frame-pointer -fno-strict-aliasing -O2 
#CFLAGS   := -mcfv4e -fomit-frame-pointer -fno-strict-aliasing -O2
LDFLAGS  :=

LDLIBS   := -lgem -lpng -lz -lyuv -lheif -lwebp -ljpeg -ltiff -llzma -lde265 -lx265 -lpthread 

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
	m68k-atari-mint-strip $(BIN)
	
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $(@D)

clean:
	@$(RM) -rv $(BIN) $(OBJ_DIR)

-include $(OBJ:.o=.d)