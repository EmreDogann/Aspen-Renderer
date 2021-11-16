cmake_minimum_required(VERSION 3.1)

project(willow-vulkan-renderer LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_PREFIX_PATH "C:/Libraries/glm/cmake/glm;C:/Libraries/glfw-3.3.5")

# Enable c++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED)
find_package(GLFW REQUIRED)
find_package(glm REQUIRED)

# Not recommended by the creators of CMake: https://stackoverflow.com/questions/1027247/is-it-better-to-specify-source-files-with-glob-or-each-file-individually-in-cmak/1060061#1060061
# file(GLOB SOURCES
#     *.h
#     *.cpp
# )

# Many sources instead recommend listing each file explicitly.
add_executable(willow-test
    main.cpp
    first_app.hpp
    first_app.cpp
    aspen_window.hpp
    aspen_window.cpp
)

target_link_libraries(willow-test PRIVATE
Vulkan::Vulkan
GLFW::GLFW
glm::glm
)

target_include_directories(willow-test PUBLIC "${PROJECT_SOURCE_DIR}")