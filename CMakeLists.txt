cmake_minimum_required(VERSION 3.7)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Toolchain_clang-mingw.cmake")

# Use ccahce if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message("Using ccache!")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif(CCACHE_PROGRAM)

set(NAME aspen-vulkan-renderer)
project(${NAME} VERSION 0.10.0)

message(STATUS "using ${CMAKE_GENERATOR}")

# 1. Set VULKAN_SDK_PATH in .env.cmake to target specific vulkan version
if (DEFINED VULKAN_SDK_PATH)
  set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include") # 1.1 Make sure this include path is correct
  set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib") # 1.2 Make sure lib path is correct
  set(Vulkan_FOUND "True")
else()
  find_package(Vulkan REQUIRED FATAL_ERROR) # throws error if could not find Vulkan
endif()

if (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
endif()

# Find all .cpp files in source directory (including those inside subdirectories).
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_executable(${PROJECT_NAME}
  ${SOURCES}
  "include/imguizmo/ImGuizmo.cpp"
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

# Precompiled Header
target_precompile_headers(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/src/pch.h")

# Disable precompiled headers for the following files.
# set_source_files_properties("include/imguizmo/**.cpp" PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

# Download All Git Submodules
# find_package(Git QUIET)
# if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
#   # Update submodules as needed
#   option(GIT_SUBMODULE "Check submodules during build" OFF)
#   if(GIT_SUBMODULE)
#     message(STATUS "Updating Submodules...")
#     execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} RESULT_VARIABLE GIT_SUBMODULE_RESULT)
#     if (NOT GIT_SUBMODULE_RESULT EQUAL "0")
#       message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMODULE_RESULT}, please checkout submodules.")
#     endif()
#   endif()
# endif()

# # Check all submodules exist for submodules that use cmake
# if(NOT EXISTS "${PROJECT_SOURCE_DIR}/include/glfw/CMakeLists.txt")
#   message(FATAL_ERROR "The GLFW submodule was not downloaded! GIT_SUBMODULE was turned off or failed.")
# endif()
# if(NOT EXISTS "${PROJECT_SOURCE_DIR}/include/glm/CMakeLists.txt")
#   message(FATAL_ERROR "The GLM submodule was not downloaded! GIT_SUBMODULE was turned off or failed.")
# endif()
# if(NOT EXISTS "${PROJECT_SOURCE_DIR}/include/entt/CMakeLists.txt")
#   message(FATAL_ERROR "The entt submodule was not downloaded! GIT_SUBMODULE was turned off or failed.")
# endif()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("include/glfw")

add_subdirectory("include/glm")

add_subdirectory("include/entt")

add_subdirectory("include/tinyobjloader")

# Compile ImGui as a static library
file(GLOB IMGUI_SOURCES "include/imgui/*.cpp")
add_library(ImGui STATIC ${IMGUI_SOURCES})

target_sources(ImGui PRIVATE
  "include/imgui/backends/imgui_impl_glfw.cpp"
  "include/imgui/backends/imgui_impl_vulkan.cpp"
)

target_include_directories(ImGui PUBLIC
  "include/imgui"
  "include/imgui/backends"
  "include/glfw/include"
  ${Vulkan_INCLUDE_DIRS}
)

if (WIN32)
  message(STATUS "CREATING BUILD FOR WINDOWS")
  
  # Include all external dependencies' header files and tell cmake where their link libraries are located.
  if (USE_MINGW)
    target_include_directories(${PROJECT_NAME} PUBLIC
      ${MINGW_PATH}/include
    )
    target_link_directories(${PROJECT_NAME} PUBLIC
      ${MINGW_PATH}/lib
    )
  endif()

  # Include target source as a include directory because that is where all our header files are located.
  target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${Vulkan_INCLUDE_DIRS}
    "include/imguizmo"
    )

  target_link_libraries(${PROJECT_NAME} glfw glm EnTT ImGui tinyobjloader ${Vulkan_LIBRARIES})
elseif (UNIX)
    message(STATUS "CREATING BUILD FOR UNIX")
    target_include_directories(${PROJECT_NAME} PUBLIC
      ${PROJECT_SOURCE_DIR}/src
      ${Vulkan_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME} glfw glm EnTT ${Vulkan_LIBRARIES})
endif()


############## Build SHADERS #######################

# Find all vertex and fragment sources within shaders directory
# taken from VBlancos vulkan tutorial
# https://github.com/vblanco20-1/vulkan-guide/blob/all-chapters/CMakeLists.txt
find_program(GLSL_VALIDATOR glslangValidator HINTS 
  ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} 
  /usr/bin 
  /usr/local/bin 
  ${VULKAN_SDK_PATH}/Bin
  ${VULKAN_SDK_PATH}/Bin32
  $ENV{VULKAN_SDK}/Bin/ 
  $ENV{VULKAN_SDK}/Bin32/
)

# Create ./build/assets folder
add_custom_target("Make_Assets_Folder" ALL DEPENDS ${PROJECT_NAME})

add_custom_command(TARGET Make_Assets_Folder POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/assets"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/assets/shaders"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/assets/models"
)

# install(DIRECTORY ${PROJECT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/build
#   FILES_MATCHING PATTERN "*" EXCLUDE
# )

# Get all .vert and .frag files in shaders directory
file(GLOB_RECURSE GLSL_SOURCE_FILES
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.frag"
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.vert"
)

# Add this target to the ALL target which will make sure that it is executed when build the ALL target.
add_custom_target(
  "Compile_Shaders"
  ALL
  DEPENDS ${SPIRV_BINARY_FILES} "Make_Assets_Folder"
)

# Build all shader files.
foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/build/assets/shaders/${FILE_NAME}.spv")
  add_custom_command(
    TARGET "Compile_Shaders" POST_BUILD
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
  "Copy_Assets"
  ALL
  DEPENDS "Make_Assets_Folder"
)

# Copy changed files from config to the binary folder after a successful build
set(source_dir "${PROJECT_SOURCE_DIR}/assets/models")
set(dest_dir "${PROJECT_SOURCE_DIR}/build/assets/models")
add_custom_command(TARGET "Copy_Assets"
  POST_BUILD
  COMMAND ${CMAKE_COMMAND}
    -Dsource_dir="${source_dir}"
    -Ddest_dir="${dest_dir}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/update_directory.cmake"
)