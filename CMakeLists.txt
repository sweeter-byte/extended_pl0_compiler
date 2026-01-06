cmake_minimum_required(VERSION 3.16)
project(pl0-compiler VERSION 1.0 LANGUAGES CXX)

# C++17 standard required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Core compiler library source files (shared between CLI and GUI)
set(CORE_SOURCES
    src/Common.cpp
    src/Token.cpp
    src/SourceManager.cpp
    src/Diagnostics.cpp
    src/Lexer.cpp
    src/SymbolTable.cpp
    src/Instruction.cpp
    src/Parser.cpp
    src/Interpreter.cpp
    src/Optimizer.cpp
)

# Create core library
add_library(pl0_core STATIC ${CORE_SOURCES})

target_include_directories(pl0_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(pl0_core PRIVATE
    -Wall 
    -Wextra 
    -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2>
)

# CLI executable
add_executable(pl0c src/main.cpp)

target_link_libraries(pl0c PRIVATE pl0_core)

target_compile_options(pl0c PRIVATE
    -Wall 
    -Wextra 
    -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2>
)

# Qt5 GUI (optional)
find_package(Qt5 COMPONENTS Core Widgets)

if(Qt5_FOUND)
    message(STATUS "Qt5 found, building GUI application")
    
    # Enable Qt MOC/UIC/RCC
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
    
    # GUI source files
    set(GUI_SOURCES
        gui/gui_main.cpp
        gui/MainWindow.cpp
        gui/CodeEditor.cpp
        gui/ConsoleWidget.cpp
    )
    
    # GUI executable
    add_executable(pl0gui ${GUI_SOURCES})
    
    target_link_libraries(pl0gui PRIVATE 
        pl0_core
        Qt5::Core 
        Qt5::Widgets
    )
    
    target_compile_options(pl0gui PRIVATE
        -Wall 
        -Wextra 
        -Wpedantic
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O2>
    )
    
    # Define QT_NO_KEYWORDS to avoid conflicts with 'emit'
    target_compile_definitions(pl0gui PRIVATE QT_NO_KEYWORDS)
    
    # Installation
    install(TARGETS pl0gui DESTINATION bin)
else()
    message(WARNING "Qt5 not found, GUI application will not be built")
endif()

# Installation (CLI always installed)
install(TARGETS pl0c DESTINATION bin)

# Print build type
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
