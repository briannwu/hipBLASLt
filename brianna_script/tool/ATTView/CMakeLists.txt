cmake_minimum_required(VERSION 3.5)

project(QtATT LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

#set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O1")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Charts REQUIRED)
find_package(Qt5 COMPONENTS Network REQUIRED)

file(GLOB sources_files
    src/mainwindow*
    src/code/*
    src/wave/*
    src/button/*
    src/include/*
    src/graphics/*
    src/graphics/container/*
    src/collection/*
    src/data/*
)

add_library(QtATT SHARED ${sources_files})
target_include_directories(QtATT PUBLIC src/)
target_link_libraries(QtATT PUBLIC Qt5::Widgets Qt5::Charts pthread Qt5::Network)
add_executable(attui src/main.cpp)
target_link_libraries(attui PRIVATE QtATT)

### Tests ###

enable_testing()
include(CTest)
add_executable(TestInstantiator tests/TestInstantiator.cpp)
target_include_directories(TestInstantiator PUBLIC src/)
target_link_libraries(TestInstantiator PRIVATE QtATT)
add_test(NAME open_empty COMMAND TestInstantiator)
add_test(NAME open_both COMMAND TestInstantiator "a")
add_test(NAME open_arg1 COMMAND TestInstantiator "b")
add_test(NAME open_arg2 COMMAND TestInstantiator "c")
