# NanoLOB (baseline) convenience wrapper around CMake.
#
#   make bench       build + run the order-book benchmark
#   make clean / distclean
#
# Dependencies come from vcpkg; on Windows the MinGW GCC triplet is used so the
# numbers are comparable with the refactored version:
#   vcpkg install abseil benchmark gtest boost-headers boost-intrusive \
#         boost-interprocess --triplet x64-mingw-static --host-triplet x64-mingw-static
# Pass extra cmake configure flags with CMAKE_FLAGS="-DFOO=BAR".
# Google Benchmark flags for `bench` are given without the leading dashes
# (make would take --foo as its own option), e.g.
#   make bench benchmark_repetitions=5 "benchmark_filter=<1, 10000>"
# or verbatim through BENCH_ARGS="--benchmark_repetitions=5".

CMAKE       ?= cmake
GENERATOR   ?= Ninja
BUILD_DIR   ?= build/baseline
JOBS        ?= 4
VCPKG_ROOT  ?= C:/package/vcpkg
CMAKE_FLAGS ?=
BENCH_ARGS  ?=

# Every benchmark_<flag>=<value> given on the command line becomes
# --benchmark_<flag>=<value> for the benchmark executable.
BENCH_FLAGS := $(foreach v,$(filter benchmark_%,$(.VARIABLES)),$(if $(filter command line,$(origin $v)),"--$v=$($v)")) $(BENCH_ARGS)

VCPKG_TOOLCHAIN := $(subst \,/,$(VCPKG_ROOT))/scripts/buildsystems/vcpkg.cmake

ifneq ($(JOBS),)
  JOBS_FLAG := -j $(JOBS)
else
  JOBS_FLAG :=
endif

EXE :=
PLATFORM_FLAGS :=
ifeq ($(OS),Windows_NT)
  EXE := .exe
  VCPKG_TRIPLET ?= x64-mingw-static
  # Link the runtime statically (Git for Windows ships a foreign libstdc++-6.dll
  # that may come first on PATH), and pull in libstdc++exp, which holds the
  # Windows console backend of std::print on GCC 14+.
  PLATFORM_FLAGS := -DCMAKE_CXX_COMPILER=g++ \
                    -DVCPKG_TARGET_TRIPLET=$(VCPKG_TRIPLET) \
                    -DVCPKG_HOST_TRIPLET=$(VCPKG_TRIPLET) \
                    -DCMAKE_EXE_LINKER_FLAGS=-static \
                    -DCMAKE_CXX_STANDARD_LIBRARIES=-lstdc++exp
endif

.PHONY: all configure bench clean distclean help

all: bench

## configure: generate the release build tree
configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_TOOLCHAIN_FILE=$(VCPKG_TOOLCHAIN) \
		$(PLATFORM_FLAGS) $(CMAKE_FLAGS)

## bench: build and run the order-book benchmark; its WAL and output files
## land in the build tree, not in the repository
bench: configure
	$(CMAKE) --build $(BUILD_DIR) --target benchmark_orderbook $(JOBS_FLAG)
	$(CMAKE) -E chdir $(BUILD_DIR) ./benchmark_orderbook$(EXE) $(BENCH_FLAGS)

## clean: remove build artefacts, keep the configured tree
clean:
	-$(CMAKE) --build $(BUILD_DIR) --target clean

## distclean: remove the build tree
distclean:
	-$(CMAKE) -E rm -rf $(BUILD_DIR)

help:
	@echo "targets: bench configure clean distclean"
	@echo "vars:    BUILD_DIR=...  JOBS=N  GENERATOR=...  VCPKG_ROOT=...  VCPKG_TRIPLET=...  CMAKE_FLAGS=...  BENCH_ARGS=...  benchmark_<flag>=..."
