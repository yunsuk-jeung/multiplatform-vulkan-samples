add_executable(05_hello_triangle main.cpp)
target_link_libraries(05_hello_triangle PRIVATE mpvk::mpvk)

# --- Compile GLSL shaders to SPIR-V at build time (SDK's glslc) ---
find_program(GLSLC_EXECUTABLE
  NAMES glslc
  HINTS "$ENV{VULKAN_SDK}/bin")
if(NOT GLSLC_EXECUTABLE)
  message(FATAL_ERROR "glslc not found; ensure the Vulkan SDK is on PATH / VULKAN_SDK is set")
endif()

set(_shader_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(_spv_dir ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${_spv_dir})

set(_spv_files)
foreach(_sh triangle.vert triangle.frag)
  add_custom_command(
    OUTPUT  ${_spv_dir}/${_sh}.spv
    COMMAND ${GLSLC_EXECUTABLE} ${_shader_src_dir}/${_sh} -o ${_spv_dir}/${_sh}.spv
    DEPENDS ${_shader_src_dir}/${_sh}
    COMMENT "glslc ${_sh}")
  list(APPEND _spv_files ${_spv_dir}/${_sh}.spv)
endforeach()

add_custom_target(05_shaders DEPENDS ${_spv_files})
add_dependencies(05_hello_triangle 05_shaders)

# Bake the .spv output directory in so the exe finds shaders regardless of CWD.
target_compile_definitions(05_hello_triangle PRIVATE SHADER_DIR="${_spv_dir}")
