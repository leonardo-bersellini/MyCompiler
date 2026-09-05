cmake_minimum_required(VERSION 3.16)

project(MyCompiler 
    VERSION 1.8.1
    LANGUAGES CXX
    DESCRIPTION "Bismuth: compilatore sviluppato in C++ e LLVM"
)

# NOME DEL PROGETTO : MYCOMPILER

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# BUILD CONFIGURATION

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# PROJECT CONFIGURATION

configure_file(
    ${CMAKE_SOURCE_DIR}/compiler/config/version.h.in
    ${CMAKE_BINARY_DIR}/generated/version.h
)

# FIND PACKAGES

if(NOT LLVM_DIR)
    set(LLVM_DIR "C:/llvm-install/lib/cmake/llvm" CACHE PATH "C:/llvm-install/lib/cmake/llvm/LLVMConfig.cmake")
endif()

find_package(LLVM REQUIRED CONFIG)


# NOME DEL COMPILER : BISMUTH_COMPILER

# SOURCES

add_executable(Bismuth
  main.cpp
  .gitignore

  compiler/codegen/codegenerator.cpp compiler/codegen/codegenerator.h
  compiler/constants/AbstractSintaxTree.h compiler/constants/keywords.h compiler/constants/symbols.h compiler/constants/token.h compiler/constants/types.h
  compiler/driver/compilerdriver.cpp compiler/driver/compilerdriver.h compiler/driver/flags.h compiler/driver/options.h
  compiler/driver/commandlineparser/commandlineparser.h compiler/driver/commandlineparser/commandlineparser.cpp
  compiler/errors/errorlog.cpp compiler/errors/errorlog.h
  compiler/errors/recovery/recoveryhandler.cpp compiler/errors/recovery/recoveryhandler.h
  compiler/lexer/lexer.cpp compiler/lexer/lexer.h
  compiler/parser/parser.cpp compiler/parser/parser.h
  compiler/semantics/semanticanalyzer.cpp compiler/semantics/semanticanalyzer.h
  compiler/utils/ansi/ansi.h compiler/utils/ansi/ansi.cpp
  compiler/utils/visitor/template_visitor.h

)

# COMPILE DEFINITIONS AND MACROS

target_compile_definitions(Bismuth PRIVATE
    $<$<CONFIG:Debug>:DEBUG_MACRO>
    $<$<CONFIG:Release>:RELEASE_MACRO>
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(EXE_SUFFIX "")
else()
    set(EXE_SUFFIX "_${CMAKE_BUILD_TYPE}")
endif()

set_target_properties(Bismuth PROPERTIES OUTPUT_NAME "Bismuth${EXE_SUFFIX}")

# INCLUDE DIRECTORIES

target_include_directories(Bismuth PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/compiler
  ${CMAKE_CURRENT_SOURCE_DIR}/compiler/constants
  ${CMAKE_CURRENT_SOURCE_DIR}/compiler
)

target_include_directories(Bismuth PRIVATE ${CMAKE_BINARY_DIR}/generated)

target_include_directories(Bismuth PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(Bismuth PRIVATE ${LLVM_DEFINITIONS})

# INCLUDE LLVM COMPONENTS

llvm_map_components_to_libnames(LLVM_LIBS
    Core
    Support
    IRReader
    Target
    MC
    CodeGen
    X86CodeGen
    X86Desc
    X86Info
    X86AsmParser
)

target_link_libraries(Bismuth PRIVATE ${LLVM_LIBS})


# POST-BUILD CONFIGURATION (THIRD-PARTY DLLS)

add_custom_command(TARGET Bismuth POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/third_party/lld"
        "$<TARGET_FILE_DIR:Bismuth>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/third_party/mingw-runtime"
        "$<TARGET_FILE_DIR:Bismuth>"
)

include(GNUInstallDirs)

install(TARGETS Bismuth
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
