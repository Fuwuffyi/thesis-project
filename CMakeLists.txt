cmake_minimum_required(VERSION 3.25)

# Set project name and version
project(ThesisProject VERSION 1.0.0 LANGUAGES C CXX)

# Export compile commands for Clangd lsp
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set language standards
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS OFF)

# Log compiler
message(STATUS "Detected compiler: ${CMAKE_CXX_COMPILER_ID}")

# Source files
file(GLOB_RECURSE SOURCE_FILES
   "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
add_dependencies(${PROJECT_NAME}
   compile_shaders
   copy_textures
   copy_meshes
   copy_gl_shaders
)

# Setup resource copying
if(CMAKE_CONFIGURATION_TYPES)  # Multi-config (Visual Studio, Xcode)
   set(RESOURCE_OUT_ROOT "${CMAKE_BINARY_DIR}/$<CONFIG>")
else()  # Single-config (Makefile, Ninja)
   set(RESOURCE_OUT_ROOT "${CMAKE_BINARY_DIR}")
endif()

set(TEXTURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/textures")
set(TEXTURE_OUT_DIR "${RESOURCE_OUT_ROOT}/resources/textures")
file(GLOB_RECURSE TEXTURE_FILES
   "${TEXTURE_DIR}/*.png"
   "${TEXTURE_DIR}/*.jpg"
)

set(COPIED_TEXTURE_FILES "")
foreach(TEXTURE_FILE ${TEXTURE_FILES})
   get_filename_component(FILE_NAME ${TEXTURE_FILE} NAME)
   set(DEST_FILE "${TEXTURE_OUT_DIR}/${FILE_NAME}")
   add_custom_command(
      OUTPUT ${DEST_FILE}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${TEXTURE_OUT_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TEXTURE_FILE} ${DEST_FILE}
      DEPENDS ${TEXTURE_FILE}
      COMMENT "Copying texture ${FILE_NAME}"
      VERBATIM
   )
   list(APPEND COPIED_TEXTURE_FILES ${DEST_FILE})
endforeach()
add_custom_target(copy_textures ALL DEPENDS ${COPIED_TEXTURE_FILES})

set(MESH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/meshes")
set(MESH_OUT_DIR "${RESOURCE_OUT_ROOT}/resources/meshes")
file(GLOB_RECURSE MESH_FILES
   "${MESH_DIR}/*.obj"
   "${MESH_DIR}/*.fbx"
)
set(COPIED_MESH_FILES "")
foreach(MESH_FILE ${MESH_FILES})
   get_filename_component(FILE_NAME ${MESH_FILE} NAME)
   set(DEST_FILE "${MESH_OUT_DIR}/${FILE_NAME}")
   add_custom_command(
      OUTPUT ${DEST_FILE}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${MESH_OUT_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MESH_FILE} ${DEST_FILE}
      DEPENDS ${MESH_FILE}
      COMMENT "Copying mesh ${FILE_NAME}"
      VERBATIM
   )
   list(APPEND COPIED_MESH_FILES ${DEST_FILE})
endforeach()
add_custom_target(copy_meshes ALL DEPENDS ${COPIED_MESH_FILES})

# Setup shader compilation
set(GL_SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/gl")
set(VK_SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/vk")
set(GL_SHADER_BINARY_DIR "${RESOURCE_OUT_ROOT}/resources/shaders/gl")
set(VK_SHADER_BINARY_DIR "${RESOURCE_OUT_ROOT}/resources/shaders/vk")

file(GLOB_RECURSE GL_GLSL_SHADERS
   "${GL_SHADER_SOURCE_DIR}/*.vert"
   "${GL_SHADER_SOURCE_DIR}/*.frag"
)

file(GLOB_RECURSE VK_GLSL_SHADERS
   "${VK_SHADER_SOURCE_DIR}/*.vert"
   "${VK_SHADER_SOURCE_DIR}/*.frag"
)

# Find program to compile shaders
find_program(GLSLANG_VALIDATOR glslangValidator HINTS ENV PATH)
if(NOT GLSLANG_VALIDATOR)
   message(FATAL_ERROR "glslangValidator not found! Please install Vulkan SDK and ensure it's in your PATH.")
endif()

# Compile each shader to SPIR-V
set(COMPILED_SPV_FILES "")
foreach(GLSL_FILE ${VK_GLSL_SHADERS})
   get_filename_component(FILE_NAME ${GLSL_FILE} NAME)
   set(SPV_FILE "${VK_SHADER_BINARY_DIR}/${FILE_NAME}.spv")
   add_custom_command(
      OUTPUT ${SPV_FILE}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${VK_SHADER_BINARY_DIR}
      COMMAND ${GLSLANG_VALIDATOR} -V ${GLSL_FILE} -o ${SPV_FILE}
      DEPENDS ${GLSL_FILE}
      COMMENT "Compiling ${FILE_NAME} to SPIR-V for Vulkan"
      VERBATIM
   )
   list(APPEND COMPILED_SPV_FILES ${SPV_FILE})
endforeach()
add_custom_target(compile_shaders ALL DEPENDS ${COMPILED_SPV_FILES})

# Copy shaders to resources
set(COPIED_GLSL_FILES "")
foreach(GLSL_FILE ${GL_GLSL_SHADERS})
   get_filename_component(FILE_NAME ${GLSL_FILE} NAME)
   set(DEST_FILE "${GL_SHADER_BINARY_DIR}/${FILE_NAME}")
   add_custom_command(
      OUTPUT ${DEST_FILE}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${GL_SHADER_BINARY_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GLSL_FILE} ${DEST_FILE}
      DEPENDS ${GLSL_FILE}
      COMMENT "Copying ${FILE_NAME} for OpenGL"
      VERBATIM
   )
   list(APPEND COPIED_GLSL_FILES ${DEST_FILE})
endforeach()
add_custom_target(copy_gl_shaders ALL DEPENDS ${COPIED_GLSL_FILES})

# Build configuration
# Debug is default
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
   set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
   set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Multi-config generators for Visual Studio
if(CMAKE_CONFIGURATION_TYPES)
   set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel" CACHE STRING "" FORCE)
endif()

# Compiler-specific flags and optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   # MSVC specific settings
   add_compile_options(
      /W0
      /permissive-
      /Zc:__cplusplus
      /Zc:preprocessor
      /source-charset:utf-8
      /w14242
      /w14254
      /w14263
      /w14265
      /w14287
      /we4289
      /w14296
      /w14311
      /w14545
      /w14546
      /w14547
      /w14549
      /w14555
      /w14640
      /w14826
      /w14905
      /w14906
      /w14928
   )
   # MSVC settings
   set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /RTC1 /MDd")
   set(CMAKE_C_FLAGS_DEBUG "/Od /Zi /RTC1 /MDd")
   set(CMAKE_CXX_FLAGS_RELEASE "/O3 /DNDEBUG /MD")
   set(CMAKE_C_FLAGS_RELEASE "/O3 /DNDEBUG /MD")
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /Zi /DNDEBUG /MD")
   set(CMAKE_C_FLAGS_RELWITHDEBINFO "/O2 /Zi /DNDEBUG /MD")
   # Disable specific MSVC warnings
   add_compile_definitions(
      _CRT_SECURE_NO_WARNINGS
      _SCL_SECURE_NO_WARNINGS
      NOMINMAX
   )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   # GCC specific settings
   target_compile_options(${PROJECT_NAME} PRIVATE
      $<$<COMPILE_LANGUAGE:CXX>:
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Wshadow
      -Wnon-virtual-dtor
      -Wold-style-cast
      -Woverloaded-virtual
      -Wnull-dereference
      -Wdouble-promotion
      -Wformat=2
      -Wimplicit-fallthrough
      -Wundef
      -Wcast-align
      -Wswitch-enum
      >
   )
   target_compile_options(${PROJECT_NAME} PRIVATE
      $<$<COMPILE_LANGUAGE:C>:
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Wshadow
      -Wundef
      -Wcast-align
      -Wswitch-enum
      >
   )

   # GCC settings
   set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -fno-omit-frame-pointer")
   set(CMAKE_C_FLAGS_DEBUG "-g3 -O0 -fno-omit-frame-pointer")
   set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math -funroll-loops")
   set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math -funroll-loops")
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
   set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
   set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
   set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
   # Enable sanitizers for GCC debug builds
   if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      include(CheckCXXCompilerFlag)
      check_cxx_compiler_flag("-fsanitize=address" HAS_ASAN)
      check_cxx_compiler_flag("-fsanitize=undefined" HAS_UBSAN)
      if(HAS_ASAN AND HAS_UBSAN)
         add_compile_options(-fsanitize=address -fsanitize=undefined)
         add_link_options(-fsanitize=address -fsanitize=undefined)
      endif()
   endif()
   # GCC-specific linker flags for release builds
   set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,-O1 -Wl,--as-needed -Wl,--strip-all")
   set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-Wl,-O1 -Wl,--as-needed -Wl,--strip-all")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   # Clang specific settings
   target_compile_options(${PROJECT_NAME} PRIVATE
      $<$<COMPILE_LANGUAGE:CXX>:
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Wshadow
      -Wnon-virtual-dtor
      -Wold-style-cast
      -Woverloaded-virtual
      -Wnull-dereference
      -Wdouble-promotion
      -Wformat=2
      -Wimplicit-fallthrough
      -Wundef
      -Wcast-align
      -Wswitch-enum
      >
   )
   target_compile_options(${PROJECT_NAME} PRIVATE
      $<$<COMPILE_LANGUAGE:C>:
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Wshadow
      -Wundef
      -Wcast-align
      -Wswitch-enum
      >
   )
   # Clang Debug settings
   set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls")
   set(CMAKE_C_FLAGS_DEBUG "-g3 -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls")
   if(NOT WIN32)
      set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math -funroll-loops -flto=thin")
      set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math -funroll-loops -flto=thin")
   else()
      set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math -funroll-loops")
      set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math -funroll-loops")
   endif()
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -fno-omit-frame-pointer")
   set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -fno-omit-frame-pointer")
   if(NOT WIN32)
      set(CMAKE_CXX_FLAGS_MINSIZEREL "-Oz -DNDEBUG -flto=thin")
      set(CMAKE_C_FLAGS_MINSIZEREL "-Oz -DNDEBUG -flto=thin")
   else()
      set(CMAKE_CXX_FLAGS_MINSIZEREL "-Oz -DNDEBUG")
      set(CMAKE_C_FLAGS_MINSIZEREL "-Oz -DNDEBUG")
   endif()
   # Enable sanitizers for Clang debug builds
   if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT WIN32)
      add_compile_options(-fsanitize=address -fsanitize=undefined)
      add_link_options(-fsanitize=address -fsanitize=undefined)
   endif()
   # Use libc++ on non-Windows platforms if available
   if(NOT WIN32)
      include(CheckCXXCompilerFlag)
      check_cxx_compiler_flag("-stdlib=libc++" HAS_LIBCXX)
      if(HAS_LIBCXX)
         add_compile_options(-stdlib=libc++)
         add_link_options(-stdlib=libc++)
      endif()
   endif()
   # Clang-specific linker flags for release builds (non-Windows)
   if(NOT WIN32)
      set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto=thin -Wl,-O1")
      set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-flto=thin -Wl,-O1 -Wl,--strip-all")
   endif()
else()
   message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}. Using generic settings.")
endif()

# Include FetchContent for automatic dependency management
include(FetchContent)

# Set the dependencies to be downloaded in parallel
set(FETCHCONTENT_QUIET FALSE)

# Platform-specific system libraries
if(!WIN32)
   # Unix-like systems (Linux, macOS)
   find_package(PkgConfig QUIET)
   # Linux-specific libraries
   if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      find_package(X11)
      if(X11_FOUND)
         message(STATUS "X11 found - enabling X11 support")
      endif()
      # Check for Wayland
      if(PkgConfig_FOUND)
         pkg_check_modules(WAYLAND wayland-client)
         if(WAYLAND_FOUND)
            message(STATUS "Wayland found - enabling Wayland support")
         endif()
      endif()
   endif()
endif()

# Vulkan
find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
   message(FATAL_ERROR "Vulkan SDK not found. Please install Vulkan SDK.")
endif()

# OpenGL
find_package(OpenGL REQUIRED)
if(NOT OpenGL_FOUND)
   message(FATAL_ERROR "OpenGL not found.")
endif()

# GLAD loader
fetchContent_Declare(
   glad
   GIT_REPOSITORY https://github.com/Dav1dde/glad.git
   GIT_TAG v2.0.8
   GIT_PROGRESS TRUE
   SOURCE_SUBDIR cmake
)
FetchContent_MakeAvailable(glad)
glad_add_library(glad_gl_core_46 STATIC REPRODUCIBLE LOADER API gl:core=4.6)

# GLFW
FetchContent_Declare(
   glfw
   GIT_REPOSITORY https://github.com/glfw/glfw.git
   GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Platform-specific GLFW settings
if(WIN32)
   set(GLFW_USE_HYBRID_HPG ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
   glm
   GIT_REPOSITORY https://github.com/g-truc/glm.git
   GIT_TAG 1.0.1
)

FetchContent_MakeAvailable(glm)

# ASSIMP
FetchContent_Declare(
   assimp
   GIT_REPOSITORY https://github.com/assimp/assimp.git
   GIT_TAG v5.4.3
)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_PLY_IMPORTER OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_STL_IMPORTER OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(assimp)

# STB
FetchContent_Declare(
   stb
   GIT_REPOSITORY https://github.com/nothings/stb.git
   GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# vk bootstrap
FetchContent_Declare(
   vk-bootstrap
   GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
   GIT_TAG v1.4.311
)
set(VKB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(VKB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(VKB_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(vk-bootstrap)

# Vulkan Memory Allocaotr
FetchContent_Declare(
   vulkan-vma
   GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
   GIT_TAG v3.3.0
)
FetchContent_MakeAvailable(vulkan-vma)

# ImGui
FetchContent_Declare(
   imgui
   GIT_REPOSITORY https://github.com/ocornut/imgui.git
   GIT_TAG v1.92.2b
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
   ${imgui_SOURCE_DIR}/imgui.cpp
   ${imgui_SOURCE_DIR}/imgui_demo.cpp
   ${imgui_SOURCE_DIR}/imgui_draw.cpp
   ${imgui_SOURCE_DIR}/imgui_tables.cpp
   ${imgui_SOURCE_DIR}/imgui_widgets.cpp
   ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
   ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
   ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
   ${imgui_SOURCE_DIR}
   ${imgui_SOURCE_DIR}/backends
)

if(WIN32)
   target_link_libraries(imgui PUBLIC glfw OpenGL::GL Vulkan::Vulkan)
else()
   target_link_libraries(imgui PUBLIC glfw OpenGL::GL Vulkan::Vulkan)
   if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND X11_FOUND)
      target_link_libraries(imgui PUBLIC ${X11_LIBRARIES})
   endif()
endif()

target_compile_definitions(imgui PUBLIC
   IMGUI_IMPL_OPENGL_LOADER_GLAD
   $<$<CONFIG:Debug>:IMGUI_DEBUG_PARANOID>
)

# Create STB library wrapper
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
   Vulkan::Vulkan
   vk-bootstrap::vk-bootstrap
   VulkanMemoryAllocator
   OpenGL::GL
   glad_gl_core_46
   glfw
   glm::glm
   assimp
   stb
   imgui
   $<$<PLATFORM_ID:Linux>:${CMAKE_DL_LIBS}>
   $<$<PLATFORM_ID:Linux>:pthread>
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
   $<$<CONFIG:Debug>:DEBUG_BUILD>
   $<$<CONFIG:Debug>:_DEBUG>
   $<$<CONFIG:Release>:RELEASE_BUILD>
   $<$<CONFIG:Release>:NDEBUG>
   $<$<CONFIG:RelWithDebInfo>:RELEASE_BUILD>
   $<$<CONFIG:RelWithDebInfo>:NDEBUG>
   $<$<CONFIG:MinSizeRel>:RELEASE_BUILD>
   $<$<CONFIG:MinSizeRel>:NDEBUG>
   $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
   $<$<PLATFORM_ID:Windows>:NOMINMAX>
   $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0601>
)

# Compiler-specific target options and optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   target_compile_options(${PROJECT_NAME} PRIVATE
      $<$<CONFIG:Debug>:/RTC1>
      $<$<CONFIG:Debug>:/JMC>
      $<$<CONFIG:Debug>:/ZI>
      $<$<CONFIG:Release>:/GL>
      $<$<CONFIG:Release>:/Gy>
      $<$<CONFIG:Release>:/GS->
      $<$<CONFIG:RelWithDebInfo>:/Gy>
      $<$<CONFIG:MinSizeRel>:/GL>
      $<$<CONFIG:MinSizeRel>:/Gy>
   )
   target_link_options(${PROJECT_NAME} PRIVATE
      $<$<CONFIG:Release>:/LTCG>
      $<$<CONFIG:Release>:/OPT:REF>
      $<$<CONFIG:Release>:/OPT:ICF>
      $<$<CONFIG:Debug>:/INCREMENTAL>
      $<$<CONFIG:MinSizeRel>:/LTCG>
      $<$<CONFIG:MinSizeRel>:/OPT:REF>
      $<$<CONFIG:MinSizeRel>:/OPT:ICF>
   )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   target_compile_options(${PROJECT_NAME} PRIVATE
      $<$<CONFIG:Debug>:-fno-limit-debug-info>
      $<$<CONFIG:Release>:-fslp-vectorize>
   )
   if(NOT WIN32)
      target_link_options(${PROJECT_NAME} PRIVATE
         $<$<CONFIG:Release>:-flto=thin>
         $<$<CONFIG:MinSizeRel>:-flto=thin>
      )
   endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   target_compile_options(${PROJECT_NAME} PRIVATE
      $<$<CONFIG:Release>:-ftree-vectorize>
      $<$<CONFIG:Release>:-fipa-pta>
   )
   target_link_options(${PROJECT_NAME} PRIVATE
      $<$<CONFIG:Release>:-flto>
      $<$<CONFIG:MinSizeRel>:-flto>
   )
endif()

if(WIN32 AND CMAKE_GENERATOR MATCHES "Visual Studio")
   set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
   set_target_properties(${PROJECT_NAME} PROPERTIES
      VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
      VS_DEBUGGER_COMMAND_ARGUMENTS ""
      VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%"
   )
   source_group("Source Files" FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
   set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# Disable library warnings
if(MSVC)
   set(THIRD_PARTY_WARN_FLAG "/W0")
else()
   set(THIRD_PARTY_WARN_FLAG "-w")
endif()

function(_disable_warnings_for target)
   # If the target doesn't exist at all, skip early.
   if(NOT TARGET ${target})
      message(STATUS "Third-party target '${target}' does not exist — skipping.")
      return()
   endif()
   # Start by assuming the target is the resolved target.
   set(_resolved_target "${target}")
   # If the target is an ALIAS, try to resolve it to the real target name.
   # get_property returns "NOTFOUND" if the property doesn't exist; otherwise it returns the aliased target name.
   get_property(_aliased TARGET ${target} PROPERTY ALIASED_TARGET)
   if(NOT "${_aliased}" STREQUAL "NOTFOUND" AND NOT "${_aliased}" STREQUAL "")
      set(_resolved_target "${_aliased}")
      message(STATUS "Resolved alias ${target} -> ${_aliased}")
   endif()
   # Final existence check
   if(NOT TARGET ${_resolved_target})
      message(STATUS "Resolved target '${_resolved_target}' not found — skipping.")
      return()
   endif()
   # Don't attempt to call set_property on an ALIAS target; check the type of the resolved target.
   get_target_property(_final_type ${_resolved_target} TYPE)
   if("${_final_type}" STREQUAL "ALIAS")
      message(WARNING "Resolved target ${_resolved_target} is an ALIAS — skipping to avoid set_property on ALIAS.")
      return()
   endif()
   # Apply the appropriate compile-option property depending on target kind.
   # INTERFACE_LIBRARY targets should receive INTERFACE_COMPILE_OPTIONS so consumers pick it up,
   # while regular library/executable targets receive COMPILE_OPTIONS.
   if("${_final_type}" STREQUAL "INTERFACE_LIBRARY")
      set_property(TARGET ${_resolved_target} APPEND PROPERTY INTERFACE_COMPILE_OPTIONS "${THIRD_PARTY_WARN_FLAG}")
   else()
      set_property(TARGET ${_resolved_target} APPEND PROPERTY COMPILE_OPTIONS "${THIRD_PARTY_WARN_FLAG}")
   endif()
   # Convert interface include directories to system include directories where present (best-effort).
   get_target_property(_iface_incs ${_resolved_target} INTERFACE_INCLUDE_DIRECTORIES)
   if(NOT _iface_incs STREQUAL "NOTFOUND" AND NOT "${_iface_incs}" STREQUAL "")
      # APPEND so we don't clobber other properties. This is safe for imported targets too.
      set_property(TARGET ${_resolved_target} APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_iface_incs}")
   endif()
   # Also try to capture non-interface include directories and mark them as system for consumers
   get_target_property(_all_incs ${_resolved_target} INCLUDE_DIRECTORIES)
   if(NOT _all_incs STREQUAL "NOTFOUND" AND NOT "${_all_incs}" STREQUAL "")
      set_property(TARGET ${_resolved_target} APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_all_incs}")
   endif()
   message(STATUS "Disabled warnings for third-party target: ${_resolved_target} (type=${_final_type})")
endfunction()

_disable_warnings_for(glad_gl_core_46)
_disable_warnings_for(glfw)
_disable_warnings_for(glm::glm)
_disable_warnings_for(assimp)
_disable_warnings_for(stb)
_disable_warnings_for(imgui)
_disable_warnings_for(OpenGL::GL)
_disable_warnings_for(Vulkan::Vulkan)
_disable_warnings_for(vk-bootstrap::vk-bootstrap)

if(WIN32 AND CMAKE_GENERATOR MATCHES "Visual Studio")
   set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
   set_target_properties(${PROJECT_NAME} PROPERTIES
      VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
      VS_DEBUGGER_COMMAND_ARGUMENTS ""
      VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%"
   )
   source_group("Source Files" FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
   set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

