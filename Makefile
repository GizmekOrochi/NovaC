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

# The compilation asset is parked for a future redesign: it must not be
# compiled into or linked with the core NovaC library.
LIB_SRC := $(filter-out src/core/novac/assets/compilation/% src/core/novac/engine/compilation/% src/core/novac/engine/backend/%,$(shell find src/core -name '*.cpp' ! -path '*/assets/*/compilation/*' | sort))
LIB_OBJ := $(patsubst src/%.cpp,build/obj/%.o,$(LIB_SRC))
LIB_SOURCE_DIRS := $(shell find src/core -type d | sort)
# The parked compilation tests are retained but must not be built/linked.
TEST_SRC := $(shell find tests -name '*.cpp' ! -path 'tests/package/*' ! -path 'tests/assets/compilation/*' | sort)
TEST_OBJ := $(patsubst tests/%.cpp,build/test-obj/%.o,$(TEST_SRC))
ASAN_LIB_OBJ := $(patsubst src/%.cpp,build/asan/lib/%.o,$(LIB_SRC))
ASAN_TEST_OBJ := $(patsubst tests/%.cpp,build/asan/tests/%.o,$(TEST_SRC))
UBSAN_LIB_OBJ := $(patsubst src/%.cpp,build/ubsan/lib/%.o,$(LIB_SRC))
UBSAN_TEST_OBJ := $(patsubst tests/%.cpp,build/ubsan/tests/%.o,$(TEST_SRC))

# Opt-in compilation implementation and its tests. No Compilation source or
# tests are linked into libNovaC.a or the default test binary.
COMPILATION_SRC := $(shell find src/core/novac/assets/compilation src/core/novac/assets/atomic/compilation src/core/novac/assets/types/compilation -name '*.cpp' | sort)
COMPILATION_TEST_SRC := tests/main.cpp \
    tests/assets/compilation/Compilation_isolated_tests.cpp \
    tests/assets/compilation/ModularCompilation_prototype_tests.cpp \
    tests/assets/compilation/X86_64_backend_tests.cpp \
    tests/assets/compilation/MachineRecipe_tests.cpp \
    tests/assets/compilation/MachineLayout_tests.cpp
COMPILATION_OBJ := $(patsubst src/%.cpp,build/compilation/obj/%.o,$(COMPILATION_SRC))
COMPILATION_TEST_OBJ := $(patsubst tests/%.cpp,build/compilation/test-obj/%.o,$(COMPILATION_TEST_SRC))
COMPILATION_TEST_BIN := bin/tests-compilation
COMPILATION_LIB := lib/libNovaCCompilation.a

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

# Derive the archive version from the public C++ version declaration.
VERSION := $(shell sed -n 's/^#define NOVAC_VERSION_STRING "\([^"]*\)"/\1/p' src/include/novac/Version.hpp)
DIST_DIR := dist
DIST_NAME := NovaC-$(VERSION)
DIST_ZIP := $(DIST_DIR)/$(DIST_NAME).zip

all: check-compilation-isolation

check-version:
	@test -n "$(VERSION)" || { echo "Error: missing NOVAC_VERSION_STRING in Version.hpp"; exit 1; }
	@grep -Fxq 'set(PACKAGE_VERSION "$(VERSION)")' cmake/NovaCConfigVersion.cmake || { \
		echo "Error: CMake package version disagrees with Version.hpp ($(VERSION))."; \
		exit 1; \
	}

# Directory mtimes track added/removed .cpp files. Recreate the archive
# when the source set changes so stale ar members cannot survive a refactor.
$(LIB): $(LIB_OBJ) $(LIB_SOURCE_DIRS) | check-compilation-source
	@mkdir -p $(dir $@)
	@rm -f $@
	$(AR) rcs $@ $(LIB_OBJ)
	$(RANLIB) $@

build/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(LIB_OBJ:.o=.d)

# Fail closed if the old compilation code is reintroduced, linked, or installed.
check-compilation-source:
	@$(PYTHON) tests/hardening/check_compilation_isolation.py

check-compilation-isolation: $(LIB)
	@$(PYTHON) tests/hardening/check_compilation_isolation.py --library "$(LIB)"

check-public-api:
	@if [ -e src/include/NovaCCompiler.hpp ]; then \
		echo "Error: obsolete src/include/NovaCCompiler.hpp detected."; \
		echo "NovaC exposes a single public umbrella header: <NovaC.hpp>."; \
		echo "Remove the obsolete header or extract NovaC into a clean directory."; \
		exit 1; \
	fi
	@if grep -R --include='*.cpp' --include='*.hpp' -n 'NovaCCompiler.hpp' tests 2>/dev/null | grep -q .; then \
		echo "Error: tests still reference obsolete <NovaCCompiler.hpp>."; \
		grep -R --include='*.cpp' --include='*.hpp' -n 'NovaCCompiler.hpp' tests 2>/dev/null || true; \
		echo "Use <NovaC.hpp> only. Your source tree contains stale files from an older NovaC revision."; \
		exit 1; \
	fi

tests: check-public-api check-compilation-isolation $(TEST_BIN)

$(TEST_BIN): $(LIB) $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(TEST_OBJ) $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

build/test-obj/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -Itests $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(TEST_OBJ:.o=.d)

# Compilation-specific tests live under tests/assets/compilation, never in the
# production asset directories. Explicit opt-in build, retaining isolation.
build/compilation/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

build/compilation/test-obj/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -Itests $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(COMPILATION_OBJ:.o=.d) $(COMPILATION_TEST_OBJ:.o=.d)

$(COMPILATION_TEST_BIN): $(LIB) $(COMPILATION_OBJ) $(COMPILATION_TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(COMPILATION_TEST_OBJ) $(COMPILATION_OBJ) $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(COMPILATION_LIB): $(COMPILATION_OBJ)
	@mkdir -p $(dir $@)
	@rm -f $@
	$(AR) rcs $@ $(COMPILATION_OBJ)
	$(RANLIB) $@

# Do not install Compilation by default. An interpreter-only consumer sees
# only NovaC::NovaC. This target is explicitly opted into by the installer.
install-compilation: install $(COMPILATION_LIB)
	@find src/include/novac/assets -type f \
	  \( -path '*/compilation/*' -o -path '*/assets/compilation/*' \) -print | \
	  while IFS= read -r file; do \
	    relative=$${file#src/include/}; \
	    $(INSTALL) -d "$(DESTDIR)$(INCLUDEDIR)/$$(dirname "$$relative")"; \
	    $(INSTALL) -m 0644 "$$file" "$(DESTDIR)$(INCLUDEDIR)/$$relative"; \
	  done
	$(INSTALL) -m 0644 $(COMPILATION_LIB) $(DESTDIR)$(LIBDIR)/libNovaCCompilation.a
	$(INSTALL) -m 0644 cmake/NovaCCompilationTargets.cmake $(DESTDIR)$(CMAKEDIR)/NovaCCompilationTargets.cmake

package-compilation-smoke: $(COMPILATION_LIB) check-version
	@rm -rf build/package-compilation-smoke
	@$(MAKE) install-compilation DESTDIR="$(abspath build/package-compilation-smoke/stage)" PREFIX=/usr CXX="$(CXX)"
	@$(CMAKE) -S tests/package/compilation -B build/package-compilation-smoke/consumer \
	  -DCMAKE_PREFIX_PATH="$(abspath build/package-compilation-smoke/stage)/usr" -DCMAKE_CXX_COMPILER="$(CXX)"
	@$(CMAKE) --build build/package-compilation-smoke/consumer --parallel 2
	@build/package-compilation-smoke/consumer/novac-compilation-consumer
	@echo "[NovaC Compilation] Optional package smoke test passed."

test-compilation: check-public-api check-version check-compilation-isolation $(COMPILATION_TEST_BIN)
	./$(COMPILATION_TEST_BIN)

# Opt-in sanitizer builds for Compilation (the linked Core library is built
# normally; tests target the Compilation implementation and extensions).
COMPILATION_ASAN_OBJ := $(patsubst src/%.cpp,build/compilation/asan/obj/%.o,$(COMPILATION_SRC))
COMPILATION_ASAN_TEST_OBJ := $(patsubst tests/%.cpp,build/compilation/asan/tests/%.o,$(COMPILATION_TEST_SRC))
COMPILATION_UBSAN_OBJ := $(patsubst src/%.cpp,build/compilation/ubsan/obj/%.o,$(COMPILATION_SRC))
COMPILATION_UBSAN_TEST_OBJ := $(patsubst tests/%.cpp,build/compilation/ubsan/tests/%.o,$(COMPILATION_TEST_SRC))

build/compilation/asan/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(ASAN_FLAGS) -MMD -MP -c $< -o $@
build/compilation/asan/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -Itests $(CXXFLAGS) $(ASAN_FLAGS) -MMD -MP -c $< -o $@
build/compilation/ubsan/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(UBSAN_FLAGS) -MMD -MP -c $< -o $@
build/compilation/ubsan/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -Itests $(CXXFLAGS) $(UBSAN_FLAGS) -MMD -MP -c $< -o $@

-include $(COMPILATION_ASAN_OBJ:.o=.d) $(COMPILATION_ASAN_TEST_OBJ:.o=.d)
-include $(COMPILATION_UBSAN_OBJ:.o=.d) $(COMPILATION_UBSAN_TEST_OBJ:.o=.d)

bin/tests-compilation-asan: $(LIB) $(COMPILATION_ASAN_OBJ) $(COMPILATION_ASAN_TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(ASAN_FLAGS) $(COMPILATION_ASAN_TEST_OBJ) $(COMPILATION_ASAN_OBJ) $(LIB) $(LDFLAGS) $(LDLIBS) -o $@
bin/tests-compilation-ubsan: $(LIB) $(COMPILATION_UBSAN_OBJ) $(COMPILATION_UBSAN_TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(UBSAN_FLAGS) $(COMPILATION_UBSAN_TEST_OBJ) $(COMPILATION_UBSAN_OBJ) $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

test-compilation-asan: check-compilation-isolation bin/tests-compilation-asan
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./bin/tests-compilation-asan
test-compilation-ubsan: check-compilation-isolation bin/tests-compilation-ubsan
	UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./bin/tests-compilation-ubsan

# Backwards-compatible name from the prototype documentation.
test-compilation-prototype: test-compilation

test: check-public-api check-version check-compilation-isolation $(TEST_BIN)
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

test-asan: check-compilation-isolation $(ASAN_TEST_BIN)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./$(ASAN_TEST_BIN)

test-ubsan: check-compilation-isolation $(UBSAN_TEST_BIN)
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

install: check-compilation-isolation package-config
	$(INSTALL) -d $(DESTDIR)$(INCLUDEDIR)
	@# Clean stale headers left by older NovaC installs.
	rm -rf "$(DESTDIR)$(INCLUDEDIR)/novac/engine/backend" \
	       "$(DESTDIR)$(INCLUDEDIR)/novac/engine/compilation" \
	       "$(DESTDIR)$(INCLUDEDIR)/novac/assets/compilation" \
	       "$(DESTDIR)$(INCLUDEDIR)/novac/assets/backend"
	@# Install only supported headers, never the parked compilation asset.
	@find src/include -type f ! -path 'src/include/novac/assets/compilation/*' ! -path 'src/include/novac/assets/*/compilation/*' -print | \
	  while IFS= read -r file; do \
	    relative=$${file#src/include/}; \
	    $(INSTALL) -d "$(DESTDIR)$(INCLUDEDIR)/$$(dirname "$$relative")"; \
	    $(INSTALL) -m 0644 "$$file" "$(DESTDIR)$(INCLUDEDIR)/$$relative"; \
	  done
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

package-smoke: check-version check-compilation-isolation package-config
	@echo "[NovaC package] Validating installed CMake package..."
	@rm -rf $(PACKAGE_SMOKE_BUILD)
	@$(MAKE) install DESTDIR="$(abspath $(PACKAGE_SMOKE_STAGE))" PREFIX=/usr CXX="$(CXX)"
	@$(PYTHON) tests/hardening/check_compilation_isolation.py --library "$(LIB)" \
	  --installed "$(abspath $(PACKAGE_SMOKE_STAGE))/usr/include"
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

release-check: check-version test test-asan test-ubsan package-smoke doc-strict
	@echo "[NovaC release] Tests, sanitizers, and strict documentation validation passed."

dist-source: check-version
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

dist: release-check dist-source

doc-clean:
	rm -rf $(DOC_OUTPUT) $(DOC_BUILD)

doc-clean-all: doc-clean
	rm -rf $(DOC_VENV)

clean:
	rm -rf bin lib build
	rm -rf $(DOC_OUTPUT) $(DOC_BUILD)