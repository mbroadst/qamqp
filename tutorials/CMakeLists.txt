function(add_tutorial)

    cmake_parse_arguments(
        ADD_TUTORIAL_PREFIX
        ""
        "NAME"
        "SOURCES"
        ${ARGN}
    )


    add_executable(${ADD_TUTORIAL_PREFIX_NAME} ${ADD_TUTORIAL_PREFIX_SOURCES})

    target_include_directories(${ADD_TUTORIAL_PREFIX_NAME}
        PRIVATE
            ${QAMQP_PATH}/src
            ${QAMQP_PATH}/tests/common
    )
    target_link_libraries(${ADD_TUTORIAL_PREFIX_NAME} qamqp-lib-static Qt5::Core Qt5::Network)

endfunction()


add_tutorial(NAME tutorial_helloworld_receive           SOURCES helloworld/receive/main.cpp)
add_tutorial(NAME tutorial_helloworld_send              SOURCES helloworld/send/main.cpp)
    
add_tutorial(NAME tutorial_pubsub_emit_log              SOURCES pubsub/emit_log/main.cpp)
add_tutorial(NAME tutorial_pubsub_receive_logs          SOURCES pubsub/receive_logs/main.cpp)
    
add_tutorial(NAME tutorial_routing_emit_log_direct      SOURCES routing/emit_log_direct/main.cpp)
add_tutorial(NAME tutorial_routing_receive_logs_direct  SOURCES routing/receive_logs_direct/main.cpp)

add_tutorial(NAME tutorial_rpc_rpc_client               SOURCES rpc/rpc_client/main.cpp rpc/rpc_client/fibonaccirpcclient.cpp rpc/rpc_client/fibonaccirpcclient.h)
add_tutorial(NAME tutorial_rpc_rpc_server               SOURCES rpc/rpc_server/main.cpp rpc/rpc_server/server.cpp rpc/rpc_server/server.h)

add_tutorial(NAME tutorial_topics_emit_log_topic        SOURCES topics/emit_log_topic/main.cpp)
add_tutorial(NAME tutorial_topics_receive_logs_topic    SOURCES topics/receive_logs_topic/main.cpp)

add_tutorial(NAME tutorial_workqueues_new_task          SOURCES workqueues/new_task/main.cpp)
add_tutorial(NAME tutorial_workqueues_worker            SOURCES workqueues/worker/main.cpp)
