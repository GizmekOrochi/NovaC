CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20 -Isrc/include -Itests

SRC = $(shell find src -name '*.cpp')
APP_SRC = $(filter-out src/main.cpp,$(SRC))
TEST_SRC = $(shell find tests -name '*.cpp')

BIN = bin/NovaC
TEST_BIN = bin/tests
ASAN_TEST_BIN = bin/tests-asan
UBSAN_TEST_BIN = bin/tests-ubsan

SANITIZER_COMMON_FLAGS = -g -fno-omit-frame-pointer
ASAN_FLAGS = $(SANITIZER_COMMON_FLAGS) -O0 -fsanitize=address
UBSAN_FLAGS = $(SANITIZER_COMMON_FLAGS) -O1 -fsanitize=undefined -fno-sanitize-recover=undefined

PYTHON ?= python3
DOXYGEN ?= doxygen
DOC_DIR = docs
DOC_CONFIG = $(DOC_DIR)/config
DOC_SOURCE = $(DOC_CONFIG)/source
DOC_OUTPUT = $(DOC_DIR)/documentation
DOC_BUILD = $(DOC_CONFIG)/_build
DOC_DOXYGEN_BUILD = $(DOC_BUILD)/doxygen
DOC_VENV = $(DOC_CONFIG)/.venv
DOC_PYTHON = $(DOC_VENV)/bin/python
DOC_SPHINX = $(DOC_VENV)/bin/sphinx-build
DOC_REQUIREMENTS = $(DOC_CONFIG)/requirements.txt
DOC_EXAMPLE_CHECK = $(DOC_CONFIG)/check_examples.py

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

$(ASAN_TEST_BIN): $(APP_SRC) $(TEST_SRC)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(ASAN_FLAGS) $^ -o $@

$(UBSAN_TEST_BIN): $(APP_SRC) $(TEST_SRC)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(UBSAN_FLAGS) $^ -o $@

test-asan: $(ASAN_TEST_BIN)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./$(ASAN_TEST_BIN)

test-ubsan: $(UBSAN_TEST_BIN)
	UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./$(UBSAN_TEST_BIN)

$(DOC_VENV)/.ready: $(DOC_REQUIREMENTS)
	@echo "[NovaC docs] Preparing Sphinx environment..."
	@$(PYTHON) -m venv $(DOC_VENV)
	@$(DOC_PYTHON) -m pip install --upgrade pip
	@$(DOC_PYTHON) -m pip install -r $(DOC_REQUIREMENTS)
	@touch $@

doc-check:
	@command -v $(DOXYGEN) >/dev/null 2>&1 || { \
		echo "Error: Doxygen is required to build the NovaC API documentation."; \
		echo "Install Doxygen, then run 'make doc' again."; \
		exit 1; \
	}

doc: doc-check $(DOC_VENV)/.ready
	@echo "[NovaC docs] Cleaning previous generated documentation..."
	@rm -rf $(DOC_OUTPUT) $(DOC_DOXYGEN_BUILD)
	@mkdir -p $(DOC_OUTPUT) $(DOC_DOXYGEN_BUILD)
	@echo "[NovaC docs] Extracting C++ API with Doxygen..."
	@cd $(DOC_CONFIG) && $(DOXYGEN) Doxyfile
	@echo "[NovaC docs] Building HTML with Sphinx..."
	@$(DOC_SPHINX) -c $(DOC_CONFIG) -b html --keep-going $(DOC_SOURCE) $(DOC_OUTPUT)
	@echo "[NovaC docs] Done: $(DOC_OUTPUT)/index.html"

doc-examples:
	@echo "[NovaC docs] Compiling and running documentation examples..."
	@$(PYTHON) $(DOC_EXAMPLE_CHECK)

doc-strict: doc-check doc-examples $(DOC_VENV)/.ready
	@echo "[NovaC docs] Cleaning previous generated documentation..."
	@rm -rf $(DOC_OUTPUT) $(DOC_DOXYGEN_BUILD)
	@mkdir -p $(DOC_OUTPUT) $(DOC_DOXYGEN_BUILD)
	@echo "[NovaC docs] Extracting C++ API with Doxygen..."
	@cd $(DOC_CONFIG) && $(DOXYGEN) Doxyfile
	@echo "[NovaC docs] Building HTML with Sphinx (warnings are errors)..."
	@$(DOC_SPHINX) -W -c $(DOC_CONFIG) -b html --keep-going $(DOC_SOURCE) $(DOC_OUTPUT)
	@echo "[NovaC docs] Strict build passed: $(DOC_OUTPUT)/index.html"

doc-clean:
	rm -rf $(DOC_OUTPUT) $(DOC_BUILD)

doc-clean-all: doc-clean
	rm -rf $(DOC_VENV)

clean:
	rm -rf bin
	rm -rf $(DOC_OUTPUT) $(DOC_BUILD)

.PHONY: all run tests test test-asan test-ubsan doc-check doc doc-examples doc-strict doc-clean doc-clean-all clean