
cmake_minimum_required(VERSION 3.15)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()


project ("NanoLOB" CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Constants
add_compile_definitions(MAX_ORDERS=2'100'000ULL) # Actually 2*N, N for bids and N for asks
add_compile_definitions(NANOSECONDS_PER_STATS_SNAPSHOT=2*10'000'000'000ULL)
add_compile_definitions(RESET_WAL_FILES=true)
add_compile_definitions(CLEAR_OUTPUT_FILES=true)
add_compile_definitions(CLEAR_LOGGER_STATS_AFTER_STAPSHOT=false) # False if you want to see full statistics



add_executable (NanoLOB "src/main.cpp" "src/record_type/Order.h" "src/bucket/Bucket.h" "src/bitset/Bitset.h" "src/store/OrderStore.h""src/OrderBook/OrderBook.h" "src/allocators/ObjectPool.h" "src/lockfree/SPSCQueue.h" "src/logger/Logger.h" "src/TradeEvent/TradeEvent.h" "src/wal/WALShared.h" "src/other/Other.h" "src/output/Outputer.h" "src/stats/CommonStats.h" "src/stats/PercentileStats.h" "src/stats/StatsTransfer.h" "src/time/Time.h" "src/time/TicksTimer.h")

# 1. Look for libs
find_package(absl REQUIRED)
find_package(Boost REQUIRED) 
find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

# 2. Creating common cfg
add_library(lob_config INTERFACE)

target_include_directories(lob_config INTERFACE src)

add_library(lob_opts INTERFACE)
if(NOT MSVC)
    target_compile_options(lob_opts INTERFACE 
        -O3 
        -march=native    
        -fno-plt         
        -Wno-interference-size 
        -flto
    )
    target_link_options(lob_opts INTERFACE -flto)
    target_compile_definitions(lob_opts INTERFACE NDEBUG) 
else()
    target_compile_options(lob_opts INTERFACE 
        $<$<CONFIG:Release>:/O2 /Ot /Oi /arch:AVX2 /GL /GT /Zi> 
    )
    target_link_options(lob_opts INTERFACE
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF /DEBUG> 
    )
endif()

target_link_libraries(lob_config INTERFACE 
    absl::flat_hash_map 
    absl::node_hash_map 
    Boost::boost

    lob_opts
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
    
    add_compile_options("$<$<CONFIG:Release>:/O2;/Ot;/Oi;/arch:AVX2;/Gy>")
    
endif()


# Tests
add_executable(unit_tests "tests/UnitTestBucket.cpp" "tests/UnitTestBits.cpp" "tests/UnitTestOrder.cpp" "tests/UnitTestOrderBook.cpp" "tests/UnitTestObjectPool.cpp" "tests/UnitTestSPSCQueue.cpp" "tests/UnitTestWALShared.cpp" "tests/UnitTestLogger.cpp")
target_compile_features(unit_tests PRIVATE cxx_std_23)
target_include_directories(unit_tests PRIVATE 
    src 
)
target_link_libraries(unit_tests PRIVATE lob_config GTest::gtest_main)

# Benchs 
add_executable(benchmarks "bench/OrderStoreBench.cpp" "bench/SPSCQueueBench.cpp")
target_link_libraries(benchmarks PRIVATE lob_config benchmark::benchmark_main)

add_executable(benchmark_orderbook "bench/OrderBookBenchBuyExisted.cpp")
target_link_libraries(benchmark_orderbook PRIVATE lob_config benchmark::benchmark_main)

