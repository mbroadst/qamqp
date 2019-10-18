if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    message(STATUS "Defaulting to 'Debug' build. Use -DCMAKE_BUILD_TYPE=Release for releases.")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message(STATUS "This is a '${CMAKE_BUILD_TYPE}' build with the '${CMAKE_GENERATOR}' generator.")
message(STATUS "The 'install' target will use the prefix '${CMAKE_INSTALL_PREFIX}'")

if ("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
    message(STATUS "Not using cross-compile toolchain. Building locally ...")
else()
    message(STATUS "Cross-compiling using toolchain file '${CMAKE_TOOLCHAIN_FILE}' ...")
endif()
