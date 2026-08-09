cmake_minimum_required(VERSION 3.16)

project(MyCompiler LANGUAGES CXX)

# NOME DEL PROGETTO : MYCOMPILER

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# BUILD CONFIGURATION

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# FIND PACKAGE

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

set(LLVM_DIR "C:/llvm-install/lib/cmake/llvm")
find_package(LLVM REQUIRED CONFIG)

# NOME DEL COMPILER : BISMUTH_COMPILER

# SOURCES

add_executable(Bismuth
  main.cpp
  .gitignore

  compiler/codegen/codegenerator.cpp compiler/codegen/codegenerator.h
  compiler/constants/AbstractSintaxTree.h compiler/constants/keywords.h compiler/constants/symbols.h compiler/constants/token.h compiler/constants/types.h
  compiler/driver/compilerdriver.cpp compiler/driver/compilerdriver.h compiler/driver/flags.h compiler/driver/options.h
  compiler/errorlog/errorlog.cpp compiler/errorlog/errorlog.h
  compiler/lexer/lexer.cpp compiler/lexer/lexer.h
  compiler/parser/parser.cpp compiler/parser/parser.h
  compiler/semantics/semanticanalyzer.cpp compiler/semantics/semanticanalyzer.h

)
target_link_libraries(Bismuth PRIVATE Qt${QT_VERSION_MAJOR}::Core ${LLVM_LIBS})

# INCLUDE DIRECTORIES

target_include_directories(Bismuth PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/compiler
  ${CMAKE_CURRENT_SOURCE_DIR}/compiler/constants
  ${CMAKE_CURRENT_SOURCE_DIR}/compiler
)

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


include(GNUInstallDirs)

install(TARGETS Bismuth
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
