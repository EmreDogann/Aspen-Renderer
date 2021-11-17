cmake_minimum_required(VERSION 3.16)

project(aspen-vulkan-renderer LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_PREFIX_PATH "C:/Libraries/glm/cmake/glm;C:/Libraries/glfw-3.3.5")

# Enable C++17 standard
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
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE src/main.cpp)
target_sources(${PROJECT_NAME} PRIVATE src/aspen_window.hpp src/aspen_window.cpp)
target_sources(${PROJECT_NAME} PRIVATE src/first_app.hpp src/first_app.cpp)
target_sources(${PROJECT_NAME} PRIVATE src/aspen_pipeline.hpp src/aspen_pipeline.cpp)
target_sources(${PROJECT_NAME} PRIVATE src/aspen_device.hpp src/aspen_device.cpp)
target_sources(${PROJECT_NAME} PRIVATE src/aspen_swap_chain.hpp src/aspen_swap_chain.cpp)

# Compile vertex shader
add_custom_target(simple_shader.vert
    COMMAND glslc shaders/simple_shader.vert -o shaders/simple_shader.vert.spv
    BYPRODUCTS ${PROJECT_SOURCE_DIR}/shaders/simple_shader.vert.spv
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# Compile fragment shader
# If this step fails, compilation fails
add_custom_target(simple_shader.frag 
    COMMAND glslc shaders/simple_shader.frag -o shaders/simple_shader.frag.spv
    BYPRODUCTS ${PROJECT_SOURCE_DIR}/shaders/simple_shader.frag.spv
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# Delete ./build/shaders folder before build 
# Allows shaders to exist only after successful build
# Does not allow stale shaders to exist after unsuccessful build
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)

# Copy shaders from ./shaders to ./build/shaders
# Allows clean shaders after successful build
# Allows nice file paths in source code
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)

add_dependencies(${PROJECT_NAME} simple_shader.vert)
add_dependencies(${PROJECT_NAME} simple_shader.frag)

target_link_libraries(${PROJECT_NAME} PRIVATE
Vulkan::Vulkan
GLFW::GLFW
glm::glm
)