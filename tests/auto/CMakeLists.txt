set(LNK $<TARGET_OBJECTS:qamqp-lib> Qt5::Core Qt5::Network)

add_qtest(NAME qamqpchannel SOURCES qamqpchannel/tst_qamqpchannel.cpp LIBS ${LNK})
add_qtest(NAME qamqpclient SOURCES qamqpclient/certs.qrc qamqpclient/tst_qamqpclient.cpp LIBS ${LNK})
add_qtest(NAME qamqpexchange SOURCES qamqpexchange/tst_qamqpexchange.cpp LIBS ${LNK})
add_qtest(NAME qamqpqueue SOURCES qamqpqueue/tst_qamqpqueue.cpp LIBS ${LNK})
