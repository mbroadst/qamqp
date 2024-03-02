enable_testing()


function(add_qtest)

    cmake_parse_arguments(
        ADD_QTEST_PREFIX
        ""
        "NAME"
        "SOURCES;LIBS"
        ${ARGN}
    )


    add_executable(${ADD_QTEST_PREFIX_NAME} ${ADD_QTEST_PREFIX_SOURCES})

    target_include_directories(${ADD_QTEST_PREFIX_NAME}
        PRIVATE
            ${QAMQP_PATH}/src
            ${QAMQP_PATH}/tests/common
    )
    target_link_libraries(${ADD_QTEST_PREFIX_NAME} ${ADD_QTEST_PREFIX_LIBS} Qt5::Test)
    _enable_compiler_coverage_flags_for(${ADD_QTEST_PREFIX_NAME})

    add_test(NAME ${ADD_QTEST_PREFIX_NAME} COMMAND ${ADD_QTEST_PREFIX_NAME})

endfunction()
