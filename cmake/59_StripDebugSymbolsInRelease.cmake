if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set(WILL_CREATE_RELEASE_WITH_SYMBOLS OFF)
    
    # define an option for that
    option(RELEASE_WITH_SYMBOLS "When building a release, generate debug symbols as well BUT strip them from the binary" OFF)
    
    # did the user request it from the cmake options?
    if(${RELEASE_WITH_SYMBOLS})
        # do we have a supported build environment?
        if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR 
            "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR 
            "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
            
            # find the required binaries
            find_program(STRIP_BINARY "${COMPILER_PREFIX}strip")
            find_program(OBJCOPY_BINARY "${COMPILER_PREFIX}objcopy")

            if ("${STRIP_BINARY}" STREQUAL "STRIP_BINARY-NOTFOUND")
                message(STATUS "ReleaseWSymbols: Did not find a 'strip' util for that toolchain!")
            else()
                if ("${OBJCOPY_BINARY}" STREQUAL "OBJCOPY_BINARY-NOTFOUND")
                    message(STATUS "ReleaseWSymbols: Did not find a 'objcopy' util for that toolchain!")
                else()
                    # if we reach here, we can enable "release with symbols"
                    set(WILL_CREATE_RELEASE_WITH_SYMBOLS ON)            
                endif()
            endif()
        else()
            message(STATUS "ReleaseWSymbols: Don't know how to strip symbols for compiler '${CMAKE_CXX_COMPILER_ID}'!")
        endif()    
    endif()
    
    # finally some debug output
    if(${WILL_CREATE_RELEASE_WITH_SYMBOLS})
        message(STATUS "ReleaseWSymbols: Will create a 'Release' with debug symbols being stripped to separate files.")

        # create binaries with debugging information, although we are building a Release
        add_definitions(-g3)
        
        # define a helper to strip the binary
        macro(_strip_debug_symbols __target)
            # first create a copy of the target file
            add_custom_command(TARGET ${__target} POST_BUILD COMMAND ${OBJCOPY_BINARY} --only-keep-debug $<TARGET_FILE:${__target}> $<TARGET_FILE:${__target}>.debug)

            # now strip its debug information from original
            add_custom_command(TARGET ${__target} POST_BUILD COMMAND ${STRIP_BINARY} -g $<TARGET_FILE:${__target}>)

            # finally install debug
            install(FILES $<TARGET_FILE:${__target}>.debug DESTINATION debug)
        endmacro()
    endif()  
endif()
