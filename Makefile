BIN = ./bin
SRC = ./src
INCLUDES = -I$(SRC)
LIBS = -lelf

all: $(BIN)/megas2

$(BIN)/megas2: $(wildcard $(SRC)/*.cpp $(SRC)/*.h)
	@mkdir -p $(BIN)
	g++ $(LIBS) -g -o $@ -Wall $(filter %.cpp,$^)

clean:
	rm -rf $(BIN)

.phony: clean
