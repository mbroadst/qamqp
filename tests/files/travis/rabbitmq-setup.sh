#!/bin/bash

echo "[
    {rabbit, [
        {ssl_listeners, [5671]},
        {ssl_options, [{cacertfile,'${TRAVIS_BUILD_DIR}/tests/files/certs/testca/cacert.pem'},
        {certfile,'${TRAVIS_BUILD_DIR}/tests/files/certs/server/cert.pem'},
        {keyfile, '${TRAVIS_BUILD_DIR}/tests/files/certs/server/key.pem'},
        {verify,verify_peer},
        {fail_if_no_peer_cert,false}]}
    ]}
]." >> rabbitmq.config

sudo CONFIG_FILE=$PWD RABBITMQ_NODENAME=test-rabbitmq rabbitmq-server -detached
