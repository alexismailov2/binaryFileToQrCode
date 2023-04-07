cmake_minimum_required(VERSION 3.16)
project(BinToPicture)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)

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