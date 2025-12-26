# Protobuf

# first try the package provided config
find_package(Protobuf CONFIG)
if(NOT Protobuf_FOUND)
  # Fall back to the CMake provide module
  find_package(Protobuf MODULE REQUIRED)
endif()

# Fix protobuf using the wrong protoc executable when cross compiling if we
# aren't using vcpkg
if(CMAKE_CROSSCOMPILING AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  message("-- Adjusting protoc executable for cross-compiling")
  unset(_PROTOBUF_PROTOC_EXECUTABLE)
  set(cmake_find_root_path ${CMAKE_FIND_ROOT_PATH})
  unset(CMAKE_FIND_ROOT_PATH)
  find_program(_PROTOBUF_PROTOC_EXECUTABLE protoc NO_CMAKE_FIND_ROOT_PATH)
  message("\t protoc executable path detected: ${_PROTOBUF_PROTOC_EXECUTABLE}")
  if(NOT TARGET protobuf::protoc)
    add_executable(protobuf::protoc IMPORTED)
  endif()
  set_target_properties(protobuf::protoc PROPERTIES
          IMPORTED_LOCATION_RELEASE "${_PROTOBUF_PROTOC_EXECUTABLE}"
          IMPORTED_LOCATION_DEBUG "${_PROTOBUF_PROTOC_EXECUTABLE}"
          IMPORTED_LOCATION_RELWIHDEBINFO "${_PROTOBUF_PROTOC_EXECUTABLE}"
          IMPORTED_LOCATION_NOCONFIG "${_PROTOBUF_PROTOC_EXECUTABLE}"
  )
  set(CMAKE_FIND_ROOT_PATH ${cmake_find_root_path})
endif() # CMAKE_CROSSCOMPILING AND NOT DEFINED VCPKG_TARGET_TRIPLET

add_library(mixxx-proto OBJECT)

if(TARGET protobuf::libprotobuf-lite)
  target_link_libraries(mixxx-proto PUBLIC protobuf::libprotobuf-lite)
elseif(TARGET protobuf::libprotobuf)
  target_link_libraries(mixxx-proto PUBLIC protobuf::libprotobuf)
else()
  message(
    FATAL_ERROR
    "Protobuf or Protobuf-lite libraries are required to compile Mixxx."
  )
endif()

if(EMSCRIPTEN)
  # If we try linking in a proto lib built without -pthread we get
  #   wasm-ld: error: --shared-memory is disallowed by keys.pb.cc.o
  #   because it was not compiled with 'atomics' or 'bulk-memory' features.
  # See https://groups.google.com/g/emscripten-discuss/c/G4nwFprZFYo
  target_compile_options(mixxx-proto PRIVATE -pthread)
endif()

protobuf_generate(
  LANGUAGE cpp
  TARGET mixxx-proto
  PROTOS
    beats.proto
    headers.proto
    keys.proto
    skin.proto
    waveform.proto
)
