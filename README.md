[![Build Status](https://travis-ci.org/mbroadst/qamqp.svg?branch=master)](https://travis-ci.org/mbroadst/qamqp)
[![Coverage Status](https://img.shields.io/coveralls/mbroadst/qamqp.svg)](https://coveralls.io/r/mbroadst/qamqp?branch=master)

QAMQP
=============
A Qt4/Qt5 implementation of AMQP 0.9.1, focusing primarily on RabbitMQ support.

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
Coming soon!


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
