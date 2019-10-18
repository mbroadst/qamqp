set(LIB_NAME qamqp-lib)

file(GLOB HDR *.h)
file(GLOB SRC *.cpp)

source_group("${LIB_NAME} Sources" ${HDR} ${SRC})

set(LNK Qt5::Core Qt5::Network)

add_library(${LIB_NAME} OBJECT ${API} ${SRC})
target_link_libraries(${LIB_NAME} ${LNK})
