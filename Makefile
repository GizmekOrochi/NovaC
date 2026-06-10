CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20 -Isrc/include

SRC = $(shell find src -name '*.cpp')
BIN = bin/NovaC

all: $(BIN)

$(BIN): $(SRC)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

run: all
	./$(BIN)

clean:
	rm -f $(BIN)

.PHONY: all run clean