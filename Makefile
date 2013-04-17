BIN = ./bin
SRC = ./src
LIB = ./lib
OBJ = ./obj
LIBS = -lelf -lSDL -lSDL_gfx -lSDL_image -lSDL_ttf -lrt
CFLAGS = -std=gnu++11 -O3 -Wall
#-fwhole-program -flto

INCLUDES = -I$(SRC)
HEADERS = $(shell find $(SRC) -name '*.h')
SOURCES = $(shell find $(SRC) -name '*.cpp')

SOURCES += $(LIB)/jsoncpp/jsoncpp.cpp
INCLUDES += -I$(LIB)/jsoncpp -I$(LIB)
CFLAGS += -DJSON_IS_AMALGAMATION

OBJS = $(patsubst %.cpp, $(OBJ)/%.o, $(SOURCES))

all: $(BIN)/megas2

obj/%.o: %.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	g++ $(CFLAGS) $(INCLUDES) -c -g -o $@ $<

$(BIN)/megas2: $(OBJS)
	@mkdir -p $(BIN)
	g++ $(CFLAGS) $(INCLUDES) -g -o $@ $^ $(LIBS)

clean:
	rm -rf $(BIN) $(OBJ)

.phony: clean
