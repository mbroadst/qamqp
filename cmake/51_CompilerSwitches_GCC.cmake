# 1) determine compiler version
execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE COMPILER_VERSION)
string(REGEX MATCHALL "[0-9]+" COMPILER_VERSION_COMPONENTS ${COMPILER_VERSION})
list(APPEND COMPILER_VERSION_COMPONENTS 0)
list(GET COMPILER_VERSION_COMPONENTS 0 COMPILER_MAJOR)
list(GET COMPILER_VERSION_COMPONENTS 1 COMPILER_MINOR)

message(STATUS "Using ${CMAKE_CXX_COMPILER} in version ${COMPILER_MAJOR}.${COMPILER_MINOR}")

# 2) control build flags according to build type
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    # debugging symbols and no optimization please
    add_definitions(-g3 -O0)
else(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    # optimize the build please
    add_definitions(-O3)
endif()

# 3) finally ensure we see many warnings
add_definitions(-Wall -Wextra)

# 4) helper for coverage instrumentation
macro(_enable_compiler_coverage_flags_for __target)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set_property(TARGET ${__target} APPEND_STRING PROPERTY COMPILE_FLAGS " -fprofile-arcs -ftest-coverage ")
        target_link_libraries(${__target} gcov)
    endif()
endmacro()
