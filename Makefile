# NanoLOB convenience wrapper around CMake.
#
#   make debug | rel-deb | release   configure + build that configuration
#   make tests                       build + run the unit tests
#   make benchs                      build + run the benchmarks
#   make clean / distclean
#
# Override the configuration used by `tests` / `benchs` with CONFIG=<name>, e.g.
#   make tests CONFIG=release
# Pass extra cmake configure flags with CMAKE_FLAGS="-DFOO=BAR".

CMAKE       ?= cmake
CTEST       ?= ctest
GENERATOR   ?= Ninja
BUILD_ROOT  ?= build
CONFIG      ?= debug
# Deliberately conservative: Ninja would otherwise spawn nproc+2 compilers and
# the dependency builds (gtest / benchmark) are memory hungry. Raise with JOBS=N.
JOBS        ?= 4
CMAKE_FLAGS ?=

ifneq ($(JOBS),)
  JOBS_FLAG := -j $(JOBS)
else
  JOBS_FLAG :=
endif

# make-level config name -> CMAKE_BUILD_TYPE
BUILD_TYPE_debug   := Debug
BUILD_TYPE_rel-deb := RelWithDebInfo
BUILD_TYPE_release := Release

EXE :=
ifeq ($(OS),Windows_NT)
  EXE := .exe
endif

.PHONY: all debug rel-deb release tests benchs run-tests run-benchs \
        configure build clean distclean help

all: debug

debug rel-deb release:
	@$(MAKE) --no-print-directory build CONFIG=$@

## configure: generate the build tree for $(CONFIG)
configure:
	$(CMAKE) -S . -B $(BUILD_ROOT)/$(CONFIG) -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE_$(CONFIG)) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		$(CMAKE_FLAGS)

## build: configure (idempotent) and compile everything for $(CONFIG)
build: configure
	$(CMAKE) --build $(BUILD_ROOT)/$(CONFIG) --config $(BUILD_TYPE_$(CONFIG)) $(JOBS_FLAG)

## tests: build and run the unit tests
tests:
	@$(MAKE) --no-print-directory configure CONFIG=$(CONFIG)
	$(CMAKE) --build $(BUILD_ROOT)/$(CONFIG) --config $(BUILD_TYPE_$(CONFIG)) --target lob_tests $(JOBS_FLAG)
	$(CTEST) --test-dir $(BUILD_ROOT)/$(CONFIG) --build-config $(BUILD_TYPE_$(CONFIG)) --output-on-failure

## benchs: build and run the benchmarks (release by default)
benchs: CONFIG := release
benchs:
	@$(MAKE) --no-print-directory configure CONFIG=$(CONFIG)
	$(CMAKE) --build $(BUILD_ROOT)/$(CONFIG) --config $(BUILD_TYPE_$(CONFIG)) --target lob_benchs $(JOBS_FLAG)
	./$(BUILD_ROOT)/$(CONFIG)/benchs/lob_benchs$(EXE)

## clean: remove build artefacts of $(CONFIG), keep the configured tree
clean:
	-$(CMAKE) --build $(BUILD_ROOT)/$(CONFIG) --target clean

## distclean: nuke every build tree (dependency downloads included)
distclean:
	-$(CMAKE) -E rm -rf $(BUILD_ROOT)

help:
	@echo "targets: debug rel-deb release tests benchs clean distclean"
	@echo "vars:    CONFIG=debug|rel-deb|release  JOBS=N  GENERATOR=...  CMAKE_FLAGS=..."
