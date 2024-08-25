# Add source to this project's executable.
add_executable(vkpr 
  "engine/vk_initializers.cpp"
  "engine/vk_images.cpp"
  "engine/vk_descriptors.cpp"
  "engine/vk_pipelines.cpp"
  "engine/vk_engine.cpp"

  "io/points.cpp"
  
  "ui/menu.cpp"
  "ui/object_controls.cpp"

  "utils/resource_management.cpp"

  "main.cpp"
)

set_property(TARGET vkpr PROPERTY CXX_STANDARD 20)
target_compile_definitions(vkpr PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_include_directories(vkpr PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(vkpr PUBLIC vma glm Vulkan::Vulkan fmt::fmt stb_image SDL2::SDL2 vkbootstrap imgui fastgltf::fastgltf
                      magic_enum tinyobjloader nfd)

target_precompile_headers(vkpr PUBLIC <optional> <vector> <memory> <string> <vector> <unordered_map> <glm/mat4x4.hpp>  <glm/vec4.hpp> <vulkan/vulkan.h>)

add_custom_command(TARGET vkpr POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:vkpr> $<TARGET_RUNTIME_DLLS:vkpr>
  COMMAND_EXPAND_LISTS
  )
  