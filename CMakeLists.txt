
cmake_minimum_required(VERSION 3.15)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()


project ("NanoLOB" CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Добавьте источник в исполняемый файл этого проекта.
add_executable (NanoLOB "src/main.cpp" "src/record_type/Order.h" "src/bucket/Bucket.h" "src/bitset/Bitset.h" "src/store/OrderStore.h"
"src/OrderBook/OrderBook.h" "src/allocators/ObjectPool.h" "src/skiplist/Skiplist.h")

# 1. Look for libs
find_package(absl REQUIRED)
find_package(Boost REQUIRED) 
find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

# 2. Создаем общую конфигурацию
add_library(lob_config INTERFACE)

target_include_directories(lob_config INTERFACE src)
target_link_libraries(lob_config INTERFACE 
    absl::flat_hash_map 
    absl::node_hash_map 
    Boost::boost
)

target_link_libraries(${PROJECT_NAME} PRIVATE lob_config)

target_include_directories(NanoLOB PRIVATE 
    src
    src/bitset
    src/bucket
    src/store
    src/allocators
)

if(MSVC)
    # /O2 - максимальная скорость
    # /Ot - приоритет скорости над размером кода
    # /arch:AVX2 - включает быстрый поиск в мапе (если проц тянет)
    # /Oi - включает интринсики (встроенные функции процессора)
    #target_compile_options(${PROJECT_NAME} PRIVATE /O2 /arch:AVX2)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()


# Tests
add_executable(unit_tests "tests/UnitTestBucket.cpp" "tests/UnitTestBits.cpp" "tests/UnitTestOrder.cpp" "tests/UnitTestOrderBook.cpp" "tests/UnitTestObjectPool.cpp" "tests/UnitTestSkiptest.cpp")
target_compile_features(unit_tests PRIVATE cxx_std_23)
target_include_directories(unit_tests PRIVATE 
    src 
)
target_link_libraries(unit_tests PRIVATE lob_config GTest::gtest_main)

# Benchs 
add_executable(benchmarks "bench/OrderBookBench.cpp" "bench/OrderBookBenchBuyExisted.cpp" "bench/IntrusiveListBench.cpp")
target_link_libraries(benchmarks PRIVATE lob_config benchmark::benchmark_main)