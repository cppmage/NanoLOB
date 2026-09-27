cmake_minimum_required(VERSION 3.14)
project(NanoLOB VERSION 1.0.0 LANGUAGES CXX)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTING "Build the testing suite" ON)
option(BUILD_BENCHMARKS "Build the benchmarks" ON)

# ---------------------------------------------------------------- dependencies
include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.85.0/boost-1.85.0-cmake.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
set(BOOST_INCLUDE_LIBRARIES intrusive)
set(BOOST_ENABLE_CMAKE ON)
FetchContent_MakeAvailable(Boost)

# ------------------------------------------------------------- the library
add_library(lob INTERFACE)
add_library(lob::lob ALIAS lob)

target_include_directories(lob INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(lob INTERFACE Boost::intrusive)

target_compile_features(lob INTERFACE cxx_std_23)

# On MinGW the executables otherwise pick up whichever libstdc++-6.dll /
# libwinpthread-1.dll happens to be first on PATH (Git for Windows ships its
# own, built by a different GCC) and crash on startup. Link the runtime in.
if(MINGW)
    target_link_options(lob INTERFACE -static-libgcc -static-libstdc++ -static)
endif()

# Constants
target_compile_definitions(lob INTERFACE
    MAX_ORDERS=2100000ULL # Actually 2*N, N for bids and N for asks
    NANOSECONDS_PER_STATS_SNAPSHOT=20000000000ULL
    RESET_WAL_FILES=true
    CLEAR_OUTPUT_FILES=true
    CLEAR_LOGGER_STATS_AFTER_SNAPSHOT=false # False if you want to see full statistics
)

# ------------------------------------------------------------------ subprojects
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchs)
endif()
