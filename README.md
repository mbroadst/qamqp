[![Build Status](https://travis-ci.org/mbroadst/qamqp.svg?branch=master)](https://travis-ci.org/mbroadst/qamqp)
[![Coverage Status](https://img.shields.io/coveralls/mbroadst/qamqp.svg)](https://coveralls.io/r/mbroadst/qamqp?branch=master)

QAMQP
=============
A Qt5/Qt6 implementation of AMQP 0.9.1, focusing primarily on RabbitMQ support.

Usage
------------
* [hello world](https://github.com/mbroadst/qamqp/tree/master/tutorials/helloworld)
* [pubsub](https://github.com/mbroadst/qamqp/tree/master/tutorials/pubsub)
* [routing](https://github.com/mbroadst/qamqp/tree/master/tutorials/routing)
* [rpc](https://github.com/mbroadst/qamqp/tree/master/tutorials/rpc)
* [topics](https://github.com/mbroadst/qamqp/tree/master/tutorials/topics)
* [work queues](https://github.com/mbroadst/qamqp/tree/master/tutorials/workqueues)

Documentation
------------
Tests checked and integrated against rabbitmq 3.11 (August 1, 2022)
Qt5.6.3 (MSVC2017) 32Bit
Qt5.15.2 (MSVC2019, MSVC2022, MinGW, Clang) 32 and 64Bit
Qt6.5 (MSVC2019, MSVC2022) 64Bit

A good starting point is:
* running a local RabbitMQ,
* browse to http://localhost:15672/#/queues  (guest/guest)
* Start the "receive" sample and see in your browser the "hello" queue appear
* publish a message there

Usage notes
------------

#### credentials
There is no default `guest`/`guest` credential. Call `setUsername()` and
`setPassword()` (or supply a `QAmqpAuthenticator` via `setAuth()`) before
connecting, or pass them in the connection URI.

#### thread affinity
A `QAmqpClient`, and every channel, exchange and queue it creates, must be used
from the thread that owns the client's event loop. Most of the API is invoked
directly rather than through queued signal/slot connections, so calling it from
another thread is not safe. Use one `QAmqpClient` per thread rather than sharing
a single client.

#### message buffering
`QAmqpQueue` derives from `QQueue<QAmqpMessage>` and buffers delivered messages in
memory. That buffer is unbounded, so a consumer that does not drain it can grow
without limit while a producer is fast. Set a prefetch window with
`QAmqpChannel::qos()` and `dequeue()` messages promptly.

Individual message bodies are bounded: a declared body larger than
`AMQP_MESSAGE_MAX` (128 MiB) is rejected instead of being accumulated.

AMQP Support
------------

#### connection
| method | supported |
| ---    | ---       |
| connection.start      | ✓ |
| connection.start-ok   | ✓ |
| connection.secure     | ✓ |
| connection.secure-ok  | ✓ |
| connection.tune       | ✓ |
| connection.tune-ok    | ✓ |
| connection.open       | ✓ |
| connection.open-ok    | ✓ |
| connection.close      | ✓ |
| connection.close-ok   | ✓ |

#### channel
| method | supported |
| ------ | --------- |
| channel.open          | ✓ |
| channel.open-ok       | ✓ |
| channel.flow          | ✓ |
| channel.flow-ok       | ✓ |
| channel.close         | ✓ |
| channel.close-ok      | ✓ |

#### exchange
| method | supported |
| ------ | --------- |
| exchange.declare      | ✓ |
| exchange.declare-ok   | ✓ |
| exchange.delete       | ✓ |
| exchange.delete-ok    | ✓ |

#### queue
| method | supported |
| ------ | --------- |
| queue.declare         | ✓ |
| queue.declare-ok      | ✓ |
| queue.bind            | ✓ |
| queue.bind-ok         | ✓ |
| queue.unbind          | ✓ |
| queue.unbind-ok       | ✓ |
| queue.purge           | ✓ |
| queue.purge-ok        | ✓ |
| queue.delete          | ✓ |
| queue.delete-ok       | ✓ |

#### basic
| method | supported |
| ------ | --------- |
| basic.qos             | ✓ |
| basic.consume         | ✓ |
| basic.consume-ok      | ✓ |
| basic.cancel          | ✓ |
| basic.cancel-ok       | ✓ |
| basic.publish         | ✓ |
| basic.return          | ✓ |
| basic.deliver         | ✓ |
| basic.get             | ✓ |
| basic.get-ok          | ✓ |
| basic.get-empty       | ✓ |
| basic.ack             | ✓ |
| basic.reject          | ✓ |
| basic.recover         | ✓ |

#### tx
| method | supported |
| ------ | --------- |
| tx.select             | X |
| tx.select-ok          | X |
| tx.commit             | X |
| tx.commit-ok          | X |
| tx.rollback           | X |
| tx.rollback-ok        | X |

#### confirm
| method | supported |
| ------ | --------- |
| confirm.select        | ✓ |
| confirm.select-ok     | ✓ |

Credits
------------
* Thank you to [@fuCtor](https://github.com/fuCtor) for the original implementation work.
