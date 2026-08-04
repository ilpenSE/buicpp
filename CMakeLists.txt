cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

project(ICL LANGUAGES CXX)

file(GLOB_RECURSE ICL_SOURCES CONFIGURE_DEPENDS
  src/*.cpp
)

add_library(icl_shared SHARED ${ICL_SOURCES})
add_library(icl_static STATIC ${ICL_SOURCES})

target_include_directories(icl_shared PUBLIC include)
target_include_directories(icl_static PUBLIC include)

set_target_properties(icl_shared PROPERTIES OUTPUT_NAME icl)

if (ICL_TARGET_MSVC)
  set_target_properties(icl_static PROPERTIES
    OUTPUT_NAME icl_static)

  target_compile_options(icl_static PRIVATE
    /permissive-
    /Zc:__cplusplus
  )

  target_compile_options(icl_shared PRIVATE
    /permissive-
    /Zc:__cplusplus
  )
else()
  set_target_properties(icl_static PROPERTIES
    OUTPUT_NAME icl)
endif()
