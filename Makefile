CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20 -Isrc/include -Itests

SRC = $(shell find src -name '*.cpp')
APP_SRC = $(filter-out src/main.cpp,$(SRC))
TEST_SRC = $(shell find tests -name '*.cpp')

BIN = bin/NovaC
TEST_BIN = bin/tests

all: $(BIN)

$(BIN): $(SRC)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $^ -o $@

tests: $(TEST_BIN)

$(TEST_BIN): $(APP_SRC) $(TEST_SRC)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $^ -o $@

run: all
	./$(BIN)

test: tests
	./$(TEST_BIN)

clean:
	rm -rf bin

.PHONY: all run tests test clean