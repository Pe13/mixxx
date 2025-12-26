# included from src/rendergraph/CMakeLists.txt

set(
  shaders
  pattern.frag
  pattern.vert
  rgb.frag
  rgb.vert
  rgba.frag
  rgba.vert
  texture.frag
  texture.vert
  unicolor.frag
  unicolor.vert
)

if(TARGET rendergraph_sg)
  qt6_add_shaders(rendergraph_sg "shaders-qsb"
      BATCHABLE
      PRECOMPILE
      # FIXME qsl relies on 'spirv-opt' to be available in the path, so only installing it via VCPKG is not sufficient
      # See https://doc.qt.io/qt-6/qtshadertools-build.html#invoking-external-tools
      # OPTIMIZED
      PREFIX
          /shaders/rendergraph
      FILES
          ${shaders}
  )
endif()

if(TARGET rendergraph_gl)
  # USE_QSHADER_FOR_GL is set in src/rendergraph/CMakeLists.txt when Qt >= 6.6
  if(USE_QSHADER_FOR_GL)
    # Add the .qsb shader bundles; rendergraph::MaterialShader will use
    # QShader to extract the GLSL shader from the bundle.
    message(STATUS "Adding qsb shaders to rendergraph_gl")
    qt6_add_shaders(rendergraph_gl "shaders-qsb"
          BATCHABLE
          PRECOMPILE
          # FIXME qsl relies on 'spirv-opt' to be available in the path, so only installing it via VCPKG is not sufficient
          # See https://doc.qt.io/qt-6/qtshadertools-build.html#invoking-external-tools
          # OPTIMIZED
          PREFIX
              /shaders/rendergraph
          FILES
              ${shaders}
    )
  else()
    # Use GLSL shaders extracted from the .qsb shader bundles using
    # generate_shaders_gl.pl
#    message(STATUS "Adding gl shaders to rendergraph_gl")
#    include(generated_shaders_gl.cmake)

#    qt_add_resources(rendergraph_gl "shaders-gl"
#          PREFIX
#              /shaders/rendergraph
#          FILES
#              ${generated_shaders_gl}
#    )
      message(STATUS "Adding gl shaders to rendergraph_gl (Manual Generation)")
      include(generated_shaders_gl.cmake)

      # 1. Paths
      set(shader_qrc_file "${CMAKE_CURRENT_BINARY_DIR}/rendergraph_shaders-gl.qrc")
      set(shader_rcc_cpp "${CMAKE_CURRENT_BINARY_DIR}/qrc_rendergraph_shaders-gl.cpp")

      # 2. Generate the .qrc file immediately during CMake configuration
      set(qrc_content "<!DOCTYPE RCC><RCC version=\"1.0\">\n<qresource prefix=\"/shaders/rendergraph\">\n")
      foreach(shader_file ${generated_shaders_gl})
        # Note: Ensure paths are absolute for the .qrc file
        get_filename_component(abs_shader_path "${shader_file}" ABSOLUTE)
        string(APPEND qrc_content "    <file>${abs_shader_path}</file>\n")
      endforeach()
      string(APPEND qrc_content "</qresource>\n</RCC>\n")
      file(WRITE "${shader_qrc_file}" "${qrc_content}")

      # 3. Define the command to run RCC
      add_custom_command(
              OUTPUT "${shader_rcc_cpp}"
              COMMAND Qt6::rcc -o "${shader_rcc_cpp}" "${shader_qrc_file}"
              DEPENDS "${shader_qrc_file}" ${generated_shaders_gl}
              COMMENT "RCC: Generating ${shader_rcc_cpp}"
              VERBATIM
      )

      # 4. Create a dedicated target for the generation
      # This prevents the "multiple targets" error by isolating the command
      add_custom_target(generate_rendergraph_gl_cpp DEPENDS "${shader_rcc_cpp}")

      # 5. Add the .cpp to the main target and link the dependency
      target_sources(rendergraph_gl PRIVATE "${shader_rcc_cpp}")
      add_dependencies(rendergraph_gl generate_rendergraph_gl_cpp)

      # Ensure the directory is included if needed
      target_include_directories(rendergraph_gl PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
endif()
