include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# NOTE: tests/legacy/ holds the pre-refactoring suite; it is intentionally not
# globbed yet. Move files back next to this CMakeLists.txt as they are ported.
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(lob_tests ${TEST_SOURCES})

target_link_libraries(lob_tests PRIVATE
    lob
    GTest::gtest_main
)

gtest_discover_tests(lob_tests)
