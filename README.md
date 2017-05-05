# libdist - C++ utilities for Distributed Computing


 distlib| --
:-------|---------------------------------:
Author  | [M. Massenzio](https://www.linkedin.com/in/mmassenzio)
Version | 0.5.1
Updated | 2016-10-09


[![badge](https://img.shields.io/badge/conan.io-glog%2F0.3.4-green.svg?logo=data:image/png;base64%2CiVBORw0KGgoAAAANSUhEUgAAAA4AAAAOCAMAAAAolt3jAAAA1VBMVEUAAABhlctjlstkl8tlmMtlmMxlmcxmmcxnmsxpnMxpnM1qnc1sn85voM91oM11oc1xotB2oc56pNF6pNJ2ptJ8ptJ8ptN9ptN8p9N5qNJ9p9N9p9R8qtOBqdSAqtOAqtR%2BrNSCrNJ/rdWDrNWCsNWCsNaJs9eLs9iRvNuVvdyVv9yXwd2Zwt6axN6dxt%2Bfx%2BChyeGiyuGjyuCjyuGly%2BGlzOKmzOGozuKoz%2BKqz%2BOq0OOv1OWw1OWw1eWx1eWy1uay1%2Baz1%2Baz1%2Bez2Oe02Oe12ee22ujUGwH3AAAAAXRSTlMAQObYZgAAAAFiS0dEAIgFHUgAAAAJcEhZcwAACxMAAAsTAQCanBgAAAAHdElNRQfgBQkREyOxFIh/AAAAiklEQVQI12NgAAMbOwY4sLZ2NtQ1coVKWNvoc/Eq8XDr2wB5Ig62ekza9vaOqpK2TpoMzOxaFtwqZua2Bm4makIM7OzMAjoaCqYuxooSUqJALjs7o4yVpbowvzSUy87KqSwmxQfnsrPISyFzWeWAXCkpMaBVIC4bmCsOdgiUKwh3JojLgAQ4ZCE0AMm2D29tZwe6AAAAAElFTkSuQmCC)](http://www.conan.io/source/glog/0.3.4/dwerner/testing)

[![badge](https://img.shields.io/badge/conan.io-OpenSSL%2F1.0.2g-green.svg?logo=data:image/png;base64%2CiVBORw0KGgoAAAANSUhEUgAAAA4AAAAOCAMAAAAolt3jAAAA1VBMVEUAAABhlctjlstkl8tlmMtlmMxlmcxmmcxnmsxpnMxpnM1qnc1sn85voM91oM11oc1xotB2oc56pNF6pNJ2ptJ8ptJ8ptN9ptN8p9N5qNJ9p9N9p9R8qtOBqdSAqtOAqtR%2BrNSCrNJ/rdWDrNWCsNWCsNaJs9eLs9iRvNuVvdyVv9yXwd2Zwt6axN6dxt%2Bfx%2BChyeGiyuGjyuCjyuGly%2BGlzOKmzOGozuKoz%2BKqz%2BOq0OOv1OWw1OWw1eWx1eWy1uay1%2Baz1%2Baz1%2Bez2Oe02Oe12ee22ujUGwH3AAAAAXRSTlMAQObYZgAAAAFiS0dEAIgFHUgAAAAJcEhZcwAACxMAAAsTAQCanBgAAAAHdElNRQfgBQkREyOxFIh/AAAAiklEQVQI12NgAAMbOwY4sLZ2NtQ1coVKWNvoc/Eq8XDr2wB5Ig62ekza9vaOqpK2TpoMzOxaFtwqZua2Bm4makIM7OzMAjoaCqYuxooSUqJALjs7o4yVpbowvzSUy87KqSwmxQfnsrPISyFzWeWAXCkpMaBVIC4bmCsOdgiUKwh3JojLgAQ4ZCE0AMm2D29tZwe6AAAAAElFTkSuQmCC)](http://www.conan.io/source/OpenSSL/1.0.2g/lasote/stable)

[![badge](https://img.shields.io/badge/conan.io-cryptopp%2F5.6.3-green.svg?logo=data:image/png;base64%2CiVBORw0KGgoAAAANSUhEUgAAAA4AAAAOCAMAAAAolt3jAAAA1VBMVEUAAABhlctjlstkl8tlmMtlmMxlmcxmmcxnmsxpnMxpnM1qnc1sn85voM91oM11oc1xotB2oc56pNF6pNJ2ptJ8ptJ8ptN9ptN8p9N5qNJ9p9N9p9R8qtOBqdSAqtOAqtR%2BrNSCrNJ/rdWDrNWCsNWCsNaJs9eLs9iRvNuVvdyVv9yXwd2Zwt6axN6dxt%2Bfx%2BChyeGiyuGjyuCjyuGly%2BGlzOKmzOGozuKoz%2BKqz%2BOq0OOv1OWw1OWw1eWx1eWy1uay1%2Baz1%2Baz1%2Bez2Oe02Oe12ee22ujUGwH3AAAAAXRSTlMAQObYZgAAAAFiS0dEAIgFHUgAAAAJcEhZcwAACxMAAAsTAQCanBgAAAAHdElNRQfgBQkREyOxFIh/AAAAiklEQVQI12NgAAMbOwY4sLZ2NtQ1coVKWNvoc/Eq8XDr2wB5Ig62ekza9vaOqpK2TpoMzOxaFtwqZua2Bm4makIM7OzMAjoaCqYuxooSUqJALjs7o4yVpbowvzSUy87KqSwmxQfnsrPISyFzWeWAXCkpMaBVIC4bmCsOdgiUKwh3JojLgAQ4ZCE0AMm2D29tZwe6AAAAAElFTkSuQmCC)](http://www.conan.io/source/cryptopp/5.6.3/riebl/testing)


# Install & Build

## Conan package

See [http://conan.io](http://conan.io) for more information.

TO build the project, you need to first donwload/build the necessary binary dependencies, as
listed in `conanfile.text`.

This is done as follows:

```bash
$ sudo -H pip install -U conan
$ mkdir .conan && cd .conan
$ conan install .. -s compiler=clang -s compiler.version=3.6 \
    -s compiler.libcxx=libstdc++11 --build=missing
```

__note__
>I've found Conan not to work terribly well inside virtualenvs - but
if you can make it work there, that'd be the preferred way.

After the dependencies are built, you can see information about them using `conan info ..`
(the commands above assume that `clang` is configured on the `PATH` and that you have
version 3.6 installed).

See also `CMakeLists.txt` for the changes necessary to add Conan's builds to the targets.


### Google Protocol Buffers

The [SWIM gossip protocol implementation](#swim_gossip_and_consensus_algorithm) makes use of
Protobuf as the serialization protocol to exchange status messages between servers.

The code in this project has been tested using
[Protocol Buffers 2.6.1](https://github.com/google/protobuf/releases/tag/v2.6.1).

After downloading and unpacking the tarball, you will need to build it and install it to the
same `${INSTALL_DIR}` as defined when invoking `cmake` (see next section).

    cd $PROTOBUF_DIR
    ./configure --prefix $INSTALL_DIR
    make
    make install

This will make it so that all headers will be installed to `$INSTALL_DIR/include` and the various
`libprotobuf*` libraries in `$INSTALL_DIR/lib` - this is what the `CMakeLists.txt` expects.

## Build & testing

To build the project, it is the usual `cmake` routine:

    $ mkdir build && cd build
    $ cmake -DINSTALL_DIR=${INSTALL_DIR} \
            -DCMAKE_CXX_COMPILER=/usr/local/bin/clang++ \
            -DCOMMON_UTILS_DIR=/path/to/commons.cmake ..
    $ cmake --build .

Finally, to run the tests:

    $ ./tests/bin/distlib_test

or to simply run a subset of the tests with full debug logging:

    $ GLOG_v=2 ./tests/bin/distlib_test --gtest_filter=SwimServer*

See also the other binaries in the `build/bin` folder for more options.


### Example use of Linux system function calls

A trivial re-implementation of the `file` Linux command is in `magic.c`: it uses the `libmagic.so`
magic file detection system library.

To build, use the following lines in a `CMakeLists.txt` file:

    add_executable(file /usr/include/linux/magic.h ${SOURCE_DIR}/magic.c)
    target_link_libraries(file magic)


# Projects

## Consistent Hashing

See the [Consistent Hash paper](http://www.cs.princeton.edu/courses/archive/fall07/cos518/papers/chash.pdf)
for more details.

The code implementation here is a simple example of how to implement a set of `buckets` so that
nodes in a distributed systems could use the consistent hashing algorithm to allow nodes to
join/leave the ring, without causing massive reshuffles of the partitioned data.

A `View` is then a collection of `Buckets`, which define how the unity circle is divided, via the
`consistent_hashing()` method and every partitioned item is allocated to a (named) `Bucket`: see
the [tests](tests/test_view.cpp) for an example of adding/removing buckets and how this only
causes a fraction of the items to be re-shuffled.


## Merkle Trees

See [this post](https://codetrips.com/2016/06/19/implementing-a-merkle-tree-in-c/) for more details.

## SWIM Gossip and Consensus algorithm

References:

 * [SWIM: Scalable Weakly-consistent Infection-style Process Group Membership Protocol](SWIM);
 * [Unreliable Distributed Failure Detectors for Reliable Systems](detectors); and
 * [A Gossip-Style Failure Detection Service](gossip)

 ### PING server

 This is based on [ZeroMQ C++ bindings](http://api.zeromq.org/2-1:zmq-cpp).

 There are currently two types of servers: one continuously listening on a given `port` and a
 client sending a one-off status update to a `destination` TCP socket.

 Starting the listening server is done like this:
 ```
    ./bin/server receive 3003
 ```
 and it will cause it to listen for incoming [`SwimStatus`](proto/swim.proto) messages on port
 `3003`; a client can then send a message update using:
 ```
    ./bin/server send tcp://localhost:3003
 ```
(obviously, change the hostname if you are running the two on separate machines/VMs/containers).


[SWIM]: https://goo.gl/VUn4iQ
[detectors]: https://goo.gl/6yuh9T
[gossip]: https://goo.gl/rxAIa6
