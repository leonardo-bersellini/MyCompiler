# TESTS MODULE FOR BISMUTH COMPILER


# INCLUDE CATCH2

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(Catch2)


# BISMUTH TEST EXECUTABLE

add_executable(bismuth_tests
    test_helpers.h
    test_lexer.cpp
    test_parser.cpp
    test_semantics.cpp
    test_codegen.cpp
    test_types.cpp
)

target_link_libraries(bismuth_tests PRIVATE
    BismuthCore
    Catch2::Catch2WithMain
)


# TEST CONFIGURATION

include(CTest)
include(Catch)
catch_discover_tests(bismuth_tests)


