[![Build Status](https://travis-ci.org/mbroadst/qamqp.svg?branch=master)](https://travis-ci.org/mbroadst/qamqp)
[![Coverage Status](https://img.shields.io/coveralls/mbroadst/qamqp.svg)](https://coveralls.io/r/mbroadst/qamqp?branch=master)

QAMQP
=============
A Qt4/Qt5 implementation of AMQP 0.9.1, focusing primarily on RabbitMQ support.


Building with CMake
------------
- ensure you have the following packages on the `PATH`
    - **CMake** >= 3.12 
    - **Qt** >= 5.9
    - **lcov**

- checkout sources
```sh
$ cd ~/src
$ git clone git@github.com:ssproessig/qamqp.git
```

- we are going to build in a separate out-of-tree build directory
    - building `Debug` build with default compiler
    ```sh
    $ mkdir -p ~/build/qamqp-debug && cd ~/build/qamqp-debug
    $ cmake ~/src/qamqp
    $ cmake --build .
    ```

    - building `Release` build with **clang** compiler and **ninja** generator
    ```sh
    $ mkdir -p ~/build/qamqp-clang-release && cd ~/build/qamqp-clang-release
    $
    $ EXPORT CXX=clang++-10
    $
    $ cmake -G Ninja ~/src/qamqp
    $ cmake --build .
    ```

- running tests with coverage from inside the build directory
```sh
$                               # ... after building, in the build directory
$ ctest -V                      # -V for verbose output of QTest
$ sh generate_coverage.sh
$ ...
$ firefox coverage/index.html
```

- building a release with **Visual Studio 2019 Win64**  (make sure to have **[OpenCppCoverage](https://github.com/OpenCppCoverage/OpenCppCoverage)** on the `PATH`)
```bat
F:\2019\qamqp>cmake G:\_projects\qamqp
...

F:\2019\qamqp>cmake --build . --parallel 8 --config RelWithDebInfo
...

F:\2019\qamqp>Opencppcoverage --export_type html:coverage --modules "qamqp*.exe" --sources G:\_projects\qamqp\src --optimized_build --cover_children  -- ctest -C RelWithDebInfo
...
... open coverage/index.html in your browser ....
...

F:\2019\qamqp>cpack -G ZIP
...
CPack: - package: F:/2019/qamqp/qamqp-0.5.0-ee2bfd8-win64.zip generated.
```


Usage
------------
Use `-DWITH_TUTORIALS=ON` to enable the tutorials in CMake.

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
