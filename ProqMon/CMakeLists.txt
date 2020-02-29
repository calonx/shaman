# CMakeList.txt : CMake project for ProqMon, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add source to this project's executable.
add_executable (ProqMon "ProqMon.cpp" "ProqMon.h" "MainWidget.cpp" "MainWidget.h")

# TODO: Add tests and install targets if needed.

include_directories("$ENV{QT_PATH}/include")
# link_directories("$ENV{QT_PATH}/lib")

# find_package(zlib REQUIRED)
# find_package(Qt5Widgets REQUIRED)
# target_link_libraries(ProqMon zlib)
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

# find_library(QT5_LIBRARIES Qt5Widgets)
target_link_libraries(ProqMon ${Qt5Core_LIBRARIES} ${Qt5Widgets_LIBRARIES})