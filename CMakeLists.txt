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
#find_package(PkgConfig REQUIRED)
#pkg_check_modules(ZBar REQUIRED IMPORTED_TARGET zbar)
#set(ZBar_LIBS PkgConfig::ZBar)
find_package(Iconv REQUIRED)

add_library(ZBar_builtin
        zbar/zbar/config.c
        zbar/zbar/decoder.c
        zbar/zbar/error.c
        zbar/zbar/image.c
        zbar/zbar/img_scanner.c
        zbar/zbar/processor.c
        zbar/zbar/refcnt.c
        zbar/zbar/scanner.c
        zbar/zbar/symbol.c
        zbar/zbar/decoder/qr_finder.c
        zbar/zbar/decoder/codabar.c
        zbar/zbar/decoder/code93.c
        zbar/zbar/decoder/code39.c
        zbar/zbar/decoder/code128.c
        zbar/zbar/decoder/databar.c
        zbar/zbar/decoder/ean.c
        zbar/zbar/decoder/i25.c
        zbar/zbar/decoder/pdf417.c
        #zbar/zbar/decoder/sq_finder.c
        zbar/zbar/qrcode/qrdec.c
        zbar/zbar/qrcode/qrdectxt.c
        zbar/zbar/qrcode/rs.c
        zbar/zbar/qrcode/isaac.c
        zbar/zbar/qrcode/bch15_5.c
        zbar/zbar/qrcode/binarize.c
        zbar/zbar/qrcode/util.c)

target_compile_definitions(ZBar_builtin
        PRIVATE
        -DHAVE_INTTYPES
        -DHAVE_TIME)

message(STATUS "Iconv_INCLUDE_DIRS=${Iconv_INCLUDE_DIRS}")
message(STATUS "Iconv_LIBRARIES=${Iconv_LIBRARIES}")

target_include_directories(ZBar_builtin
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${Iconv_INCLUDE_DIRS}
        zbar/zbar)

target_include_directories(ZBar_builtin
        PUBLIC
        zbar/include)

target_link_libraries(ZBar_builtin
        PRIVATE
        ${Iconv_LIBRARIES})

set(ZBar_LIBS ZBar_builtin)
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

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
add_executable(ScreenCapturing
        ScreenCapturing.cpp)

target_link_libraries(ScreenCapturing
        PRIVATE
        ${OpenCV_LIBS}
        ${ZBar_LIBS}
        "-framework CoreGraphics")
else()
    add_definitions(-DBUILD_WITH_X11)
    add_executable(ScreenCapturing
            ScreenCapturing.cpp)

    target_link_libraries(ScreenCapturing
            PRIVATE
            ${OpenCV_LIBS}
            ${ZBar_LIBS}
            X11
            Xext)
endif()

add_subdirectory(QtApp)
