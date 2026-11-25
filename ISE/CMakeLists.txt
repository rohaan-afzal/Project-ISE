cmake_minimum_required(VERSION 3.19)
project(ISE LANGUAGES CXX)

# Keep the Qt Creator ISE project entry point, but build the actual POS GUI
# application defined by the repository root instead of the empty Qt template.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/.." "${CMAKE_BINARY_DIR}/Project-ISE")
set_target_properties(increment111 PROPERTIES OUTPUT_NAME ISE)
