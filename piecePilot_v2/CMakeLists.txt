cmake_minimum_required(VERSION 3.16)
project(kantenAnzeigen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

qt_standard_project_setup()

file(GLOB SRC_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/*.h"
)

set(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(piecePilot ${SRC_FILES})

target_link_libraries(piecePilot PRIVATE Qt6::Widgets ${OpenCV_LIBS})
target_include_directories(piecePilot PRIVATE ${OpenCV_INCLUDE_DIRS})
target_compile_definitions(piecePilot PRIVATE BASE_DIR=\"${BASE_DIR}\")