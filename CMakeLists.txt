# Project metadata
cmake_minimum_required(VERSION 3.8)
project("vulkan_guide")

# Third-party libraries
find_package(Vulkan REQUIRED)
add_subdirectory(third_party)
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

# Set directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
add_subdirectory(src)

# Define location and extensions of shader files
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/shaders/*.comp"
    )

# Define routine for shader compilation
foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
  message(STATUS ${GLSL})
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

# Define shader compilation build target
add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )

# Add resources and shader directories compiler definitions
target_compile_definitions(
  vkpr
  PUBLIC
  "-DRESOURCES_DIR=\"${CMAKE_CURRENT_LIST_DIR}/resources/\"" "-DSHADERS_DIR=\"${CMAKE_CURRENT_LIST_DIR}/shaders/\"")
