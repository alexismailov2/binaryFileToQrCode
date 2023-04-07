cmake_minimum_required(VERSION 3.16)
project(BinToPicture)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)

option(BUILD_WITH_OpenCV "Use OpenCV library" OFF)

if(BUILD_WITH_OpenCV)
find_package(OpenCV REQUIRED)
add_definitions(-DBUILD_WITH_OpenCV)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZBar REQUIRED IMPORTED_TARGET zbar)

add_executable(ASCIIDataEncoder
        QR-Code-generator/cpp/qrcodegen.hpp
        QR-Code-generator/cpp/qrcodegen.cpp
        ASCIIDataEncoder.cpp)

target_link_libraries(ASCIIDataEncoder
        PRIVATE
        ${OpenCV_LIBS}
        PkgConfig::ZBar)

add_executable(ASCIIDataDecoder
        ASCIIDataDecoder.cpp)

target_link_libraries(ASCIIDataDecoder
        PRIVATE
        ${OpenCV_LIBS}
        PkgConfig::ZBar)

add_executable(TestASCIIDataGenerator
        TestASCIIDataGenerator.cpp)

add_subdirectory(QtApp)