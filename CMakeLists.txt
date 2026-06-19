cmake_minimum_required(VERSION 3.16)

project(MyCompiler LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_executable(MyCompiler
  main.cpp
  lexer/lexer.h lexer/lexer.cpp
  parser/parser.h parser/parser.cpp
  constants/token.h
  constants/AbstractSintaxTree.h
  errorlog/errorlog.h errorlog/errorlog.cpp
  constants/symbol.h
  semantics/semanticanalyzer.h semantics/semanticanalyzer.cpp
  constants/keywords.h
)
target_link_libraries(MyCompiler PRIVATE Qt${QT_VERSION_MAJOR}::Core)

target_include_directories(MyCompiler PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/constants
  ${CMAKE_CURRENT_SOURCE_DIR}/errorlog
)

include(GNUInstallDirs)
install(TARGETS MyCompiler
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
