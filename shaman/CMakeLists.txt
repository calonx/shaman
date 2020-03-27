# CMakeList.txt : CMake project for shaman, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(Qt5_DIR         C:/GitHub/qt5/qtbase/lib/cmake/Qt5          CACHE FILEPATH "" FORCE)
set(Qt5Core_DIR     C:/GitHub/qt5/qtbase/lib/cmake/Qt5Core      CACHE FILEPATH "" FORCE)
set(Qt5Widgets_DIR  C:/GitHub/qt5/qtbase/lib/cmake/Qt5Widgets   CACHE FILEPATH "" FORCE)
set(Qt5Gui_DIR      C:/GitHub/qt5/qtbase/lib/cmake/Qt5Gui       CACHE FILEPATH "" FORCE)
set(Qt5Test_DIR     C:/GitHub/qt5/qtbase/lib/cmake/Qt5Test      CACHE FILEPATH "" FORCE)

add_compile_definitions(QT_NO_ACCESSIBILITY)

# TODO: Add tests and install targets if needed.

include_directories("$ENV{QT_PATH}/include")
include_directories("C:/GitHub/rapidjson/include")
# link_directories("$ENV{QT_PATH}/lib")

# find_package(Qt5Widgets REQUIRED)
# target_link_libraries(shaman zlib)

find_package(ZeroMQ CONFIG REQUIRED)
find_package(czmq   CONFIG REQUIRED)
find_package(Qt5    COMPONENTS Core Widgets Test REQUIRED)

# Add source to this project's executable.
add_executable (
    shaman
    "shaman.cpp"
    "shaman.h"
    "MainWidget.cpp"
    "MainWidget.h"
    "TreeModel.h"
    "TreeModel.cpp"
    "QService/qtservice.cpp"
    "QService/qtservice_win.cpp"
    "QService/qtservice.h"
    "QService/qtservice_p.h"
)

target_link_libraries(shaman PRIVATE ${Qt5Core_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5Test_LIBRARIES})
target_link_libraries(shaman PRIVATE libzmq libzmq-static)
target_link_libraries(shaman PRIVATE czmq czmq-static)