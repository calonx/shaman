# CMakeList.txt : CMake project for shaman, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(Qt5_DIR         C:/GitHub/qt5/qtbase/lib/cmake/Qt5          CACHE FILEPATH "" FORCE)
set(Qt5Core_DIR     C:/GitHub/qt5/qtbase/lib/cmake/Qt5Core      CACHE FILEPATH "" FORCE)

include_directories("$ENV{QT_PATH}/include")

find_package(ZeroMQ CONFIG REQUIRED)
find_package(czmq   CONFIG REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)

# Add source to this project's executable.
add_executable (
    shaman_broker
    "Main.cpp"
)

target_link_libraries(shaman_broker PRIVATE ${Qt5Core_LIBRARIES})
target_link_libraries(shaman_broker PRIVATE libzmq libzmq-static)
target_link_libraries(shaman_broker PRIVATE czmq czmq-static)
