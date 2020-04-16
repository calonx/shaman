# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)
cmake_policy(SET CMP0079 NEW)

project("shaman")

# Include sub-projects.
add_subdirectory("shaman")
add_subdirectory("shaman_broker")
add_subdirectory("shaman_ctrl")
add_subdirectory("shaman_svc")
add_subdirectory("qtservice")

target_link_libraries(shaman PRIVATE qtservice)
target_link_libraries(shaman_broker PRIVATE qtservice)
target_link_libraries(shaman_ctrl PRIVATE qtservice)
target_link_libraries(shaman_svc PRIVATE qtservice)
