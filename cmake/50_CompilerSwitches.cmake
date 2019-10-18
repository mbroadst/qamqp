# settings to apply to all compilers
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# we aim for C++14
set(CMAKE_CXX_STANDARD 14)

# compiler specific settings
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    include(${CMAKE_CURRENT_LIST_DIR}/51_CompilerSwitches_GCC.cmake)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    include(${CMAKE_CURRENT_LIST_DIR}/52_CompilerSwitches_Clang.cmake)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    include(${CMAKE_CURRENT_LIST_DIR}/53_CompilerSwitches_MSVC.cmake)
endif()

# finally we support building "releases with symbol"
include(${CMAKE_CURRENT_LIST_DIR}/59_StripDebugSymbolsInRelease.cmake)
