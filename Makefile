BIN = ./bin
SRC = ./src
INCLUDES = -I$(SRC)
LIBS = -lelf

all: $(BIN)/megas2

$(BIN)/megas2: $(wildcard $(SRC)/*.cpp $(SRC)/*/*.cpp $(SRC)/*.h $(SRC)/*/*.h)
	@mkdir -p $(BIN)
	g++ $(INCLUDES) $(LIBS) -g -o $@ -Wall $(filter %.cpp,$^)

clean:
	rm -rf $(BIN)

.phony: clean
