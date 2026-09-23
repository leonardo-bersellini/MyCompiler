# BISMUTH RUNTIME LIBRARY [STATIC]

add_library(BismuthRuntime STATIC
    src/print.cpp
)

target_include_directories(BismuthRuntime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(BismuthRuntime PUBLIC cxx_std_20)