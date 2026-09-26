# BISMUTH RUNTIME LIBRARY [STATIC]


add_library(BismuthRuntime STATIC
    src/print.cpp
    src/conversions.cpp
)

target_include_directories(BismuthRuntime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(BismuthRuntime PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/runtime
)

target_compile_features(BismuthRuntime PUBLIC cxx_std_20)