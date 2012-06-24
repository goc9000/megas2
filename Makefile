BIN = ./bin
SRC = ./src
INCLUDES = -I$(SRC)
LIBS = -lelf -lSDL -lSDL_gfx -lrt
CFLAGS = -O3 -Wall
#-fwhole-program -flto

all: $(BIN)/megas2

$(BIN)/megas2: $(wildcard $(SRC)/*.cpp $(SRC)/*/*.cpp $(SRC)/*.h $(SRC)/*/*.h)
	@mkdir -p $(BIN)
	g++ $(CFLAGS) $(INCLUDES) $(LIBS) -g -o $@ -Wall $(filter %.cpp,$^)

clean:
	rm -rf $(BIN)

.phony: clean
