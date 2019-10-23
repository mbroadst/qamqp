set(LNK Qt5::Core Qt5::Network)

add_qtest(NAME qamqpchannel SOURCES qamqpchannel/tst_qamqpchannel.cpp $<TARGET_OBJECTS:qamqp-lib-coverage> LIBS ${LNK})
add_qtest(NAME qamqpclient SOURCES qamqpclient/certs.qrc qamqpclient/tst_qamqpclient.cpp $<TARGET_OBJECTS:qamqp-lib-coverage> LIBS ${LNK})
add_qtest(NAME qamqpexchange SOURCES qamqpexchange/tst_qamqpexchange.cpp $<TARGET_OBJECTS:qamqp-lib-coverage> LIBS ${LNK})
add_qtest(NAME qamqpqueue SOURCES qamqpqueue/tst_qamqpqueue.cpp $<TARGET_OBJECTS:qamqp-lib-coverage> LIBS ${LNK})

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(COVERAGE_SH ${PROJECT_BINARY_DIR}/generate_coverage.sh)
  file(WRITE  ${COVERAGE_SH} "!/bin/sh\n")
  file(APPEND ${COVERAGE_SH} "\n")
  file(APPEND ${COVERAGE_SH} "lcov --capture --directory src --output-file coverage-gcov.info \n")
  file(APPEND ${COVERAGE_SH} "lcov --output-file coverage-gcov.info --remove coverage-gcov.info '${PROJECT_BINARY_DIR}/*' '*Qt*' '*include*'\n")
  file(APPEND ${COVERAGE_SH} "\n")
  file(APPEND ${COVERAGE_SH} "genhtml coverage-gcov.info --prefix src/ --output-directory coverage\n")
endif()

