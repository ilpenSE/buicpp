# toolchains/x64-win-msvc.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER cl)
set(CMAKE_CXX_COMPILER cl)
set(CMAKE_LINKER link)
set(CMAKE_AR lib)
set(ICL_TARGET_MSVC TRUE)
