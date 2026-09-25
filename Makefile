CXX ?= g++
AR ?= ar
RANLIB ?= ranlib
INSTALL ?= install
PYTHON ?= python3
DOXYGEN ?= doxygen
CMAKE ?= cmake

CPPFLAGS ?= -Isrc/include
CXXFLAGS ?= -Wall -Wextra -std=c++20
LDFLAGS ?=
LDLIBS ?=

LIB_SRC := $(shell find src/core -name '*.cpp' | sort)
LIB_OBJ := $(patsubst src/%.cpp,build/obj/%.o,$(LIB_SRC))
TEST_SRC := $(shell find tests -name '*.cpp' ! -path 'tests/package/*' | sort)
TEST_OBJ := $(patsubst tests/%.cpp,build/test-obj/%.o,$(TEST_SRC))
ASAN_LIB_OBJ := $(patsubst src/%.cpp,build/asan/lib/%.o,$(LIB_SRC))
ASAN_TEST_OBJ := $(patsubst tests/%.cpp,build/asan/tests/%.o,$(TEST_SRC))
UBSAN_LIB_OBJ := $(patsubst src/%.cpp,build/ubsan/lib/%.o,$(LIB_SRC))
UBSAN_TEST_OBJ := $(patsubst tests/%.cpp,build/ubsan/tests/%.o,$(TEST_SRC))

LIB_DIR := lib
LIB_NAME := libNovaC.a
LIB := $(LIB_DIR)/$(LIB_NAME)
TEST_BIN := bin/tests
ASAN_TEST_BIN := bin/tests-asan
UBSAN_TEST_BIN := bin/tests-ubsan

SANITIZER_COMMON_FLAGS := -g -fno-omit-frame-pointer
ASAN_FLAGS := $(SANITIZER_COMMON_FLAGS) -O0 -fsanitize=address
UBSAN_FLAGS := $(SANITIZER_COMMON_FLAGS) -O1 -fsanitize=undefined -fno-sanitize-recover=undefined

PREFIX ?= /usr/local
DESTDIR ?=
INCLUDEDIR ?= $(PREFIX)/include
LIBDIR ?= $(PREFIX)/lib
CMAKEDIR ?= $(LIBDIR)/cmake/NovaC

PACKAGE_CONFIG := build/package/NovaCConfig.cmake
PACKAGE_VERSION := build/package/NovaCConfigVersion.cmake
PACKAGE_TARGETS := build/package/NovaCTargets.cmake

DOC_DIR := docs
DOC_CONFIG := $(DOC_DIR)/config
DOC_SOURCE := $(DOC_CONFIG)/source
DOC_OUTPUT := $(DOC_DIR)/documentation
DOC_BUILD := $(DOC_CONFIG)/_build
DOC_DOXYGEN_BUILD := $(DOC_BUILD)/doxygen
DOC_VENV := $(DOC_CONFIG)/.venv
DOC_PYTHON := $(DOC_VENV)/bin/python
DOC_SPHINX := $(DOC_VENV)/bin/sphinx-build
DOC_REQUIREMENTS := $(DOC_CONFIG)/requirements.txt
DOC_EXAMPLE_CHECK := $(DOC_CONFIG)/check_examples.py

PACKAGE_SMOKE_DIR := tests/package
PACKAGE_SMOKE_BUILD := build/package-smoke
PACKAGE_SMOKE_STAGE := $(PACKAGE_SMOKE_BUILD)/stage
PACKAGE_SMOKE_CMAKE_BUILD := $(PACKAGE_SMOKE_BUILD)/consumer

VERSION := 1.2.0
DIST_DIR := dist
DIST_NAME := NovaC-$(VERSION)
DIST_ZIP := $(DIST_DIR)/$(DIST_NAME).zip

all: $(LIB)

$(LIB): $(LIB_OBJ)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^
	$(RANLIB) $@

build/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(LIB_OBJ:.o=.d)

tests: $(TEST_BIN)

$(TEST_BIN): $(LIB) $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(TEST_OBJ) $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

build/test-obj/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -Itests $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(TEST_OBJ:.o=.d)

test: $(TEST_BIN)
	./$(TEST_BIN)

build/asan/lib/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(ASAN_FLAGS) -c $< -o $@

build/asan/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -Itests $(CXXFLAGS) $(ASAN_FLAGS) -c $< -o $@

$(ASAN_TEST_BIN): $(ASAN_LIB_OBJ) $(ASAN_TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(ASAN_FLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

build/ubsan/lib/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(UBSAN_FLAGS) -c $< -o $@

build/ubsan/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -Itests $(CXXFLAGS) $(UBSAN_FLAGS) -c $< -o $@

$(UBSAN_TEST_BIN): $(UBSAN_LIB_OBJ) $(UBSAN_TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(UBSAN_FLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

test-asan: $(ASAN_TEST_BIN)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./$(ASAN_TEST_BIN)

test-ubsan: $(UBSAN_TEST_BIN)
	UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./$(UBSAN_TEST_BIN)

package-config: $(PACKAGE_CONFIG) $(PACKAGE_VERSION) $(PACKAGE_TARGETS)

$(PACKAGE_CONFIG): cmake/NovaCConfig.cmake
	@mkdir -p $(dir $@)
	cp $< $@

$(PACKAGE_VERSION): cmake/NovaCConfigVersion.cmake
	@mkdir -p $(dir $@)
	cp $< $@

$(PACKAGE_TARGETS): cmake/NovaCTargets.cmake
	@mkdir -p $(dir $@)
	cp $< $@

install: $(LIB) package-config
	$(INSTALL) -d $(DESTDIR)$(INCLUDEDIR)
	cp -R src/include/. $(DESTDIR)$(INCLUDEDIR)/
	$(INSTALL) -d $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 0644 $(LIB) $(DESTDIR)$(LIBDIR)/$(LIB_NAME)
	$(INSTALL) -d $(DESTDIR)$(CMAKEDIR)
	$(INSTALL) -m 0644 $(PACKAGE_CONFIG) $(DESTDIR)$(CMAKEDIR)/NovaCConfig.cmake
	$(INSTALL) -m 0644 $(PACKAGE_VERSION) $(DESTDIR)$(CMAKEDIR)/NovaCConfigVersion.cmake
	$(INSTALL) -m 0644 $(PACKAGE_TARGETS) $(DESTDIR)$(CMAKEDIR)/NovaCTargets.cmake

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_NAME)
	rm -f $(DESTDIR)$(INCLUDEDIR)/NovaC.hpp
	rm -rf $(DESTDIR)$(INCLUDEDIR)/novac
	rm -rf $(DESTDIR)$(CMAKEDIR)

package-smoke: $(LIB) package-config
	@echo "[NovaC package] Validating installed CMake package..."
	@rm -rf $(PACKAGE_SMOKE_BUILD)
	@$(MAKE) install DESTDIR="$(abspath $(PACKAGE_SMOKE_STAGE))" PREFIX=/usr CXX="$(CXX)"
	@$(CMAKE) -S $(PACKAGE_SMOKE_DIR) -B $(PACKAGE_SMOKE_CMAKE_BUILD) \
		-DCMAKE_PREFIX_PATH="$(abspath $(PACKAGE_SMOKE_STAGE))/usr" \
		-DCMAKE_CXX_COMPILER="$(CXX)"
	@$(CMAKE) --build $(PACKAGE_SMOKE_CMAKE_BUILD) --parallel 2
	@$(PACKAGE_SMOKE_CMAKE_BUILD)/novac-package-smoke
	@echo "[NovaC package] Installed CMake package smoke test passed."

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

release-check: test test-asan test-ubsan package-smoke doc-strict
	@echo "[NovaC release] Tests, sanitizers, and strict documentation validation passed."

dist: release-check
	@command -v git >/dev/null 2>&1 || { echo "Error: git is required to create a source release archive."; exit 1; }
	@git diff --quiet --ignore-submodules -- && git diff --cached --quiet --ignore-submodules -- || { \
		echo "Error: tracked files contain uncommitted changes. Commit them before creating a release archive."; \
		exit 1; \
	}
	@test -z "$$(git ls-files --others --exclude-standard)" || { \
		echo "Error: untracked files are present. Commit or remove them before creating a release archive."; \
		git ls-files --others --exclude-standard; \
		exit 1; \
	}
	@mkdir -p $(DIST_DIR)
	@rm -f $(DIST_ZIP)
	@git archive --format=zip --prefix=$(DIST_NAME)/ --output=$(DIST_ZIP) HEAD
	@echo "[NovaC release] Created $(DIST_ZIP) from clean HEAD."

doc-clean:
	rm -rf $(DOC_OUTPUT) $(DOC_BUILD)

doc-clean-all: doc-clean
	rm -rf $(DOC_VENV)

clean:
	rm -rf bin lib build
	rm -rf $(DOC_OUTPUT) $(DOC_BUILD)

.PHONY: all tests test test-asan test-ubsan package-config install uninstall package-smoke doc-check doc doc-examples doc-strict release-check dist doc-clean doc-clean-all clean
