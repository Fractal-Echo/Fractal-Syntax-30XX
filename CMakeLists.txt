cmake_minimum_required(VERSION 3.24)

project(FractalSyntax30XX LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT WIN32)
  message(FATAL_ERROR "FractalSyntax30XX is a Windows-only project. Build on Windows with MSVC.")
endif()

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui")

add_executable(FractalSyntax30XX
  ConsoleIX.cpp
  Helpers.h
  Signature.cpp
  Signature.h
  mem.cpp
  mem.h
  proc.cpp
  proc.h
  "${IMGUI_DIR}/imgui.cpp"
  "${IMGUI_DIR}/imgui_draw.cpp"
  "${IMGUI_DIR}/imgui_impl_glfw.cpp"
  "${IMGUI_DIR}/imgui_impl_opengl3.cpp"
  "${IMGUI_DIR}/imgui_tables.cpp"
  "${IMGUI_DIR}/imgui_widgets.cpp"
)

target_include_directories(FractalSyntax30XX PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_definitions(FractalSyntax30XX PRIVATE
  GLEW_STATIC
  NOMINMAX
  UNICODE
  WIN32_LEAN_AND_MEAN
  _UNICODE
)

if(MSVC)
  target_compile_options(FractalSyntax30XX PRIVATE /W3)
endif()

target_link_libraries(FractalSyntax30XX PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw3.lib"
  "${CMAKE_CURRENT_SOURCE_DIR}/lib/glew32s.lib"
  opengl32
  wininet
  shell32
  user32
  gdi32
  advapi32
  winmm
  imm32
  uuid
)

set_target_properties(FractalSyntax30XX PROPERTIES
  OUTPUT_NAME "FractalSyntax30XX"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
