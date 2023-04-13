cmake_minimum_required(VERSION 3.5)
project(BinToPicture)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE ON)

option(BUILD_WITH_OpenCV "Use OpenCV library" OFF)
option(BUILD_WITH_ZBar "Use Zbar for generator" OFF)

if(BUILD_WITH_OpenCV)
find_package(OpenCV REQUIRED)
add_definitions(-DBUILD_WITH_OpenCV)
endif()

if(BUILD_WITH_ZBar)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZBar REQUIRED IMPORTED_TARGET zbar)
set(ZBar_LIBS PkgConfig::ZBar)
endif()

if(BUILD_WITH_OpenCV)
add_executable(ASCIIDataEncoder
        QR-Code-generator/cpp/qrcodegen.hpp
        QR-Code-generator/cpp/qrcodegen.cpp
        ASCIIDataEncoder.cpp)

target_link_libraries(ASCIIDataEncoder
        PRIVATE
        ${OpenCV_LIBS}
        ${ZBar_LIBS})
endif()

if(BUILD_WITH_OpenCV AND BUILD_WITH_ZBar)
add_executable(ASCIIDataDecoder
        ASCIIDataDecoder.cpp)

target_link_libraries(ASCIIDataDecoder
        PRIVATE
        ${OpenCV_LIBS}
        ${ZBar_LIBS})
endif()

add_executable(TestASCIIDataGenerator
        TestASCIIDataGenerator.cpp)

add_subdirectory(QtApp)
