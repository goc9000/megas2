BIN = ./bin
SRC = ./src
LIB = ./lib
LIBS = -lelf -lSDL -lSDL_gfx -lSDL_image -lSDL_ttf -lrt
CFLAGS = -O3 -Wall
#-fwhole-program -flto

INCLUDES = -I$(SRC)
SOURCES = $(shell find $(SRC) -name '*.cpp' -o -name '*.h')

SOURCES += $(LIB)/jsoncpp/jsoncpp.cpp
INCLUDES += -I$(LIB)/jsoncpp
CFLAGS += -DJSON_IS_AMALGAMATION

all: $(BIN)/megas2

$(BIN)/megas2: $(SOURCES)
	@mkdir -p $(BIN)
	g++ $(CFLAGS) $(INCLUDES) $(LIBS) -g -o $@ -Wall $(filter %.cpp,$^)

clean:
	rm -rf $(BIN)

.phony: clean
