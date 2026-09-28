include(FetchContent)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
# Newer MinGW headers trip benchmark's own -Werror (-Wswitch in sysinfo.cc).
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.9.1.zip
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(googlebenchmark)

# NOTE: benchs/legacy/ holds the pre-refactoring benchmarks; it is intentionally
# not globbed yet. Move files back here as they are ported.
file(GLOB BENCH_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(lob_benchs ${BENCH_SOURCES})

target_link_libraries(lob_benchs PRIVATE
    lob
    benchmark::benchmark_main
)

target_compile_options(lob_benchs PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-O3;-march=native>
)

# The order-book benchmark pulls in the logging/output pipeline, which uses
# std::print -- only available from GCC 14 / recent MSVC.
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_LIBRARIES ${LOB_STDCXX_EXP})
check_cxx_source_compiles("#include <print>
int main() { std::print(\"\"); }" LOB_HAS_STD_PRINT)
unset(CMAKE_REQUIRED_LIBRARIES)

if(LOB_HAS_STD_PRINT)
    add_executable(lob_bench_orderbook ${CMAKE_CURRENT_SOURCE_DIR}/orderbook/OrderBookBench.cpp)

    target_link_libraries(lob_bench_orderbook PRIVATE
        lob
        Boost::interprocess
        benchmark::benchmark_main
    )

    target_compile_options(lob_bench_orderbook PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-O3;-march=native>
        $<$<CXX_COMPILER_ID:MSVC>:/O2;/Ot;/Oi;/arch:AVX2>
    )
else()
    message(STATUS "lob_bench_orderbook skipped: compiler has no <print> (needs GCC 14+ or MSVC)")
endif()
