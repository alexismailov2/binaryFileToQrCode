cmake_minimum_required(VERSION 3.16)
project(BinnaryFileToQrCodeGUI)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

file(GLOB SOURCE *.cpp *.hpp)

add_executable(${PROJECT_NAME}
        ../QR-Code-generator/cpp/qrcodegen.cpp
        ${SOURCE})

#target_include_directories(${PROJECT_NAME}
#        PRIVATE
#        )

target_link_libraries(${PROJECT_NAME}
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        PkgConfig::ZBar
        ${OpenCV_LIBS}) # TODO: Should be deleted aftre test

if (UNIX)
    target_link_libraries(${PROJECT_NAME}
            pthread)
endif()

if(APPLE)
    target_link_libraries(${PROJECT_NAME}
            "-framework CoreFoundation"
            "-framework IOKit")
endif()

if(NOT UNIX)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        file(COPY "C:/Qt/Qt5.12.12_/5.12.12/mingw73_64/bin/Qt5Cored.dll" DESTINATION ${CMAKE_BINARY_DIR})
        file(COPY "C:/Qt/Qt5.12.12_/5.12.12/mingw73_64/bin/Qt5Guid.dll" DESTINATION ${CMAKE_BINARY_DIR})
        file(COPY "C:/Qt/Qt5.12.12_/5.12.12/mingw73_64/bin/Qt5Widgetsd.dll" DESTINATION ${CMAKE_BINARY_DIR})
    else()
        file(COPY "C:/Qt/Qt5.12.12_/5.12.12/mingw73_64/bin/Qt5Core.dll" DESTINATION ${CMAKE_BINARY_DIR})
        file(COPY "C:/Qt/Qt5.12.12_/5.12.12/mingw73_64/bin/Qt5Gui.dll" DESTINATION ${CMAKE_BINARY_DIR})
        file(COPY "C:/Qt/Qt5.12.12_/5.12.12/mingw73_64/bin/Qt5Widgets.dll" DESTINATION ${CMAKE_BINARY_DIR})
    endif()
    file(COPY "C:/Qt/Qt5.12.12_/5.12.12/mingw73_64/bin/libgcc_s_seh-1.dll" DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY "C:/Qt/Qt5.12.12_/5.12.12/mingw73_64/bin/libwinpthread-1.dll" DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY "C:/Qt/Qt5.12.12_/5.12.12/mingw73_64/bin/libstdc++-6.dll" DESTINATION ${CMAKE_BINARY_DIR})
endif()
