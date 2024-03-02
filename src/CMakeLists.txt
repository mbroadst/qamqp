set(LIB_NAME qamqp-lib)

file(GLOB HDR *.h)
file(GLOB SRC *.cpp)

source_group("${LIB_NAME} Sources" ${HDR} ${SRC})

_add_vcs_info_to_file(qamqpclient.cpp)

set(LNK Qt5::Core Qt5::Network)

add_library(${LIB_NAME}-static ${HDR} ${SRC})
target_link_libraries(${LIB_NAME}-static ${LNK})

add_library(${LIB_NAME} SHARED ${HDR} ${SRC})
target_link_libraries(${LIB_NAME} ${LNK})

add_library(${LIB_NAME}-coverage OBJECT ${HDR} ${SRC})
target_link_libraries(${LIB_NAME}-coverage ${LNK})
_enable_compiler_coverage_flags_for(${LIB_NAME}-coverage)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    install(TARGETS ${LIB_NAME}-static EXPORT ${PROJECT_EXPORT})
    install(TARGETS ${LIB_NAME} EXPORT ${PROJECT_EXPORT})
    install(FILES 
                qamqpauthenticator.h
                qamqpchannel.h
                qamqpclient.h
                qamqpexchange.h
                qamqpglobal.h
                qamqpmessage.h
                qamqpqueue.h
                qamqptable.h
            DESTINATION include)
endif()
