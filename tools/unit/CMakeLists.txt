message("Adding tools unit test target")

add_executable(tools-unittest
    ../src/enumparser.cpp
    unittest_enumgen.cpp
)

target_include_directories(tools-unittest PRIVATE "../../tools/src")
target_include_directories(tools-unittest PRIVATE "${PEGTL_INCLUDE}")

target_link_libraries(tools-unittest gtest gtest_main)

if (COVERAGE)
    target_add_code_coverage(tools-unittest tools)
endif()

if(CTEST_INDIVIDUALLY)
    # NOTE: discovered ctest targets below are much slower than their direct counterparts.  Ctest loads
    #       tests individually instead of just running all in a class.  This makes it take much
    #       longer for execution.  This is somewhat useful if you want to execute tests using ctest scripts, but is
    #       completely unusable on dev machine: each test takes 800ms vs 20ms, and valgrind takes 4-5s per test.
    gtest_discover_tests(tools-unittest)
else()
    # Adds the entire unittest executable as a single ctest.  Great for speed.
    add_test(all-tests tools-unittest)
endif()