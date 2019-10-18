set(LIB_NAME qamqp-lib)

file(GLOB HDR *.h)
file(GLOB SRC *.cpp)

source_group("${LIB_NAME} Sources" ${HDR} ${SRC})

_add_vcs_info_to_file(qamqpclient.cpp)

set(LNK Qt5::Core Qt5::Network)

add_library(${LIB_NAME} OBJECT ${API} ${SRC})
target_link_libraries(${LIB_NAME} ${LNK})

add_library(${LIB_NAME}-coverage OBJECT ${API} ${SRC})
target_link_libraries(${LIB_NAME}-coverage ${LNK})
_enable_compiler_coverage_flags_for(${LIB_NAME}-coverage)
