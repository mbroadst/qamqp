CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0)

project(QAMQP)

include(FindPkgConfig)
include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(SRC_DIR ${QAMQP_SOURCE_DIR}/src)
set(QAMQP_DIR ${SRC_DIR}/qamqp)

# to pick up Qt5 macros
find_package(Qt5Core)

set(libqamqp_HEADERS
  ${QAMQP_DIR}/amqp_authenticator.h
  ${QAMQP_DIR}/amqp_channel.h
  ${QAMQP_DIR}/amqp_channel_p.h
  ${QAMQP_DIR}/amqp_connection.h
  ${QAMQP_DIR}/amqp_connection_p.h
  ${QAMQP_DIR}/amqp_exchange.h
  ${QAMQP_DIR}/amqp_exchange_p.h
  ${QAMQP_DIR}/amqp_frame.h
  ${QAMQP_DIR}/amqp_global.h
  ${QAMQP_DIR}/amqp.h
  ${QAMQP_DIR}/amqp_message.h
  ${QAMQP_DIR}/amqp_p.h
  ${QAMQP_DIR}/amqp_network.h
  ${QAMQP_DIR}/amqp_queue.h
  ${QAMQP_DIR}/amqp_queue_p.h
  )
set(libqamqp_SOURCES
  ${libqamqp_HEADERS}
  ${QAMQP_DIR}/amqp_authenticator.cpp
  ${QAMQP_DIR}/amqp_channel.cpp
  ${QAMQP_DIR}/amqp_connection.cpp
  ${QAMQP_DIR}/amqp.cpp
  ${QAMQP_DIR}/amqp_exchange.cpp
  ${QAMQP_DIR}/amqp_frame.cpp
  ${QAMQP_DIR}/amqp_network.cpp
  ${QAMQP_DIR}/amqp_queue.cpp
  )

set(test_client_SOURCES
  ${SRC_DIR}/main.cpp
  ${SRC_DIR}/QamqpApp.h
  ${SRC_DIR}/pubsub/EmitLog.h
  ${SRC_DIR}/pubsub/ReceiveLog.h
  ${SRC_DIR}/routing/EmitLogDirect.h
  ${SRC_DIR}/routing/ReceiveLogDirect.h
  ${SRC_DIR}/sendreceive/Send.h
  ${SRC_DIR}/sendreceive/Receive.h
  ${SRC_DIR}/workqueues/NewTask.h
  ${SRC_DIR}/workqueues/Worker.h
  )

include_directories(${SRC_DIR})
link_directories(${QAMQP_BINARY_DIR})

add_library(qamqp SHARED ${libqamqp_SOURCES})
set_target_properties(qamqp PROPERTIES SOVERSION 0.2.0)
qt5_use_modules(qamqp Core Network)


add_executable(qamqp-test ${test_client_SOURCES})
target_link_libraries(qamqp-test qamqp)
qt5_use_modules(qamqp-test Core Network)

install(TARGETS qamqp-test qamqp
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES ${libqamqp_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qamqp)
