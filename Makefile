CXX ?= g++
AR ?= ar
RANLIB ?= ranlib
INSTALL ?= install
CMAKE ?= cmake

CPPFLAGS ?= -Isrc/include
CXXFLAGS ?= -Wall -Wextra -std=c++20
LDFLAGS ?=
LDLIBS ?=

LIB_SRC := $(shell find src/core/novac/engine -name '*.cpp' | sort)
LIB_OBJ := $(patsubst src/%.cpp,build/obj/%.o,$(LIB_SRC))
LIB_SOURCE_DIRS := $(shell find src/core/novac/engine -type d | sort)
TEST_SRC := $(shell find tests/engine tests/hardening -name '*.cpp' | sort) tests/main.cpp
TEST_OBJ := $(patsubst tests/%.cpp,build/test-obj/%.o,$(TEST_SRC))
ASAN_LIB_OBJ := $(patsubst src/%.cpp,build/asan/lib/%.o,$(LIB_SRC))
ASAN_TEST_OBJ := $(patsubst tests/%.cpp,build/asan/tests/%.o,$(TEST_SRC))
UBSAN_LIB_OBJ := $(patsubst src/%.cpp,build/ubsan/lib/%.o,$(LIB_SRC))
UBSAN_TEST_OBJ := $(patsubst tests/%.cpp,build/ubsan/tests/%.o,$(TEST_SRC))

LIB := lib/libNovaC.a
TEST_BIN := bin/tests
ASAN_TEST_BIN := bin/tests-asan
UBSAN_TEST_BIN := bin/tests-ubsan
ASAN_FLAGS := -g -O0 -fno-omit-frame-pointer -fsanitize=address
UBSAN_FLAGS := -g -O1 -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=undefined

PREFIX ?= /usr/local
DESTDIR ?=
INCLUDEDIR ?= $(PREFIX)/include
LIBDIR ?= $(PREFIX)/lib
CMAKEDIR ?= $(LIBDIR)/cmake/NovaC
PACKAGE_CONFIG := build/package/NovaCConfig.cmake
PACKAGE_VERSION := build/package/NovaCConfigVersion.cmake
PACKAGE_TARGETS := build/package/NovaCTargets.cmake


VERSION := $(shell sed -n 's/^#define NOVAC_VERSION_STRING "\([^"]*\)"/\1/p' src/include/novac/Version.hpp)
DIST_NAME := NovaC-$(VERSION)
DIST_ZIP := dist/$(DIST_NAME).zip

.PHONY: all check-version check-public-api tests test test-asan test-ubsan package-config install uninstall package-smoke release-check dist-source dist clean
all: $(LIB)

check-version:
	@test -n "$(VERSION)" || { echo "Missing version"; exit 1; }
	@grep -Fxq 'set(PACKAGE_VERSION "$(VERSION)")' cmake/NovaCConfigVersion.cmake

check-public-api:
	@test ! -e src/include/NovaCCompiler.hpp || { echo "Obsolete umbrella header"; exit 1; }

$(LIB): $(LIB_OBJ) $(LIB_SOURCE_DIRS)
	@mkdir -p $(dir $@)
	@rm -f $@
	$(AR) rcs $@ $(LIB_OBJ)
	$(RANLIB) $@

build/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@
-include $(LIB_OBJ:.o=.d)

$(TEST_BIN): $(LIB) $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(TEST_OBJ) $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

build/test-obj/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -Itests $(CXXFLAGS) -MMD -MP -c $< -o $@
-include $(TEST_OBJ:.o=.d)

tests: check-public-api $(TEST_BIN)
test: check-version tests
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
test-asan: $(ASAN_TEST_BIN)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./$(ASAN_TEST_BIN)

build/ubsan/lib/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(UBSAN_FLAGS) -c $< -o $@
build/ubsan/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -Itests $(CXXFLAGS) $(UBSAN_FLAGS) -c $< -o $@
$(UBSAN_TEST_BIN): $(UBSAN_LIB_OBJ) $(UBSAN_TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(UBSAN_FLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
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
	$(INSTALL) -d "$(DESTDIR)$(INCLUDEDIR)" "$(DESTDIR)$(LIBDIR)" "$(DESTDIR)$(CMAKEDIR)"
	@find src/include -type f -print | while IFS= read -r file; do \
	  relative=$${file#src/include/}; \
	  $(INSTALL) -d "$(DESTDIR)$(INCLUDEDIR)/$$(dirname "$$relative")"; \
	  $(INSTALL) -m 0644 "$$file" "$(DESTDIR)$(INCLUDEDIR)/$$relative"; \
	done
	$(INSTALL) -m 0644 $(LIB) "$(DESTDIR)$(LIBDIR)/libNovaC.a"
	$(INSTALL) -m 0644 $(PACKAGE_CONFIG) "$(DESTDIR)$(CMAKEDIR)/NovaCConfig.cmake"
	$(INSTALL) -m 0644 $(PACKAGE_VERSION) "$(DESTDIR)$(CMAKEDIR)/NovaCConfigVersion.cmake"
	$(INSTALL) -m 0644 $(PACKAGE_TARGETS) "$(DESTDIR)$(CMAKEDIR)/NovaCTargets.cmake"

uninstall:
	rm -f "$(DESTDIR)$(LIBDIR)/libNovaC.a" "$(DESTDIR)$(INCLUDEDIR)/NovaC.hpp"
	rm -rf "$(DESTDIR)$(INCLUDEDIR)/novac" "$(DESTDIR)$(CMAKEDIR)"

package-smoke: check-version package-config
	@rm -rf build/package-smoke
	@$(MAKE) install DESTDIR="$(abspath build/package-smoke/stage)" PREFIX=/usr CXX="$(CXX)"
	@$(CMAKE) -S tests/package -B build/package-smoke/consumer \
	  -DCMAKE_PREFIX_PATH="$(abspath build/package-smoke/stage)/usr" -DCMAKE_CXX_COMPILER="$(CXX)"
	@$(CMAKE) --build build/package-smoke/consumer --parallel 2
	@build/package-smoke/consumer/novac-package-smoke

release-check: check-version test test-asan test-ubsan package-smoke

dist-source: check-version
	@mkdir -p dist
	@$(PYTHON) scripts/create_dist.py "$(DIST_ZIP)" "$(DIST_NAME)"
dist: release-check dist-source

clean:
	rm -rf bin lib build
