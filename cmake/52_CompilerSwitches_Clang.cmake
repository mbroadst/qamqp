# control build flags according to build type
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    # debugging symbols and no optimization please
    add_definitions(-g3 -O0)

    # some extended clang analyzers - MUST be enabled manually via CMake parameter
    if(${CLANG_ANALYSIS})
        set(CLANG_ANALYZERS address)

        message(STATUS "enabling clang analysis code generation: ${CLANG_ANALYZERS}")

        set(CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS}         -fsanitize=${CLANG_ANALYZERS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -fsanitize=${CLANG_ANALYZERS}")
        set(CMAKE_LINKER           "${CXX_COMPILER}")
    endif()

else(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    # optimize the build please
    add_definitions(-g -O3)
endif()

# set warning options
if (DEFINED $ENV{ENV_CLANG_CXXFLAGS})
    add_definitions($ENV{ENV_CLANG_CXXFLAGS})
else()
    add_definitions(-Weverything 
        -Wno-weak-vtables -Wno-padded -Wno-packed 
        -Wno-c++98-compat-pedantic -Wno-c++98-compat 
        -Wno-exit-time-destructors -Wno-missing-prototypes 
        -Wno-documentation-unknown-command
        -Wno-used-but-marked-unused
        -Wno-gnu-zero-variadic-macro-arguments
        -Wno-disabled-macro-expansion
        -Wno-global-constructors
    )

    # clang 3.9 introduced a new warning that needs to be checked - apple clang does not yet support it
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "AppleClang" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER "3.8")
        add_definitions(-Wno-undefined-func-template)
    endif()
endif()
