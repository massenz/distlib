# libdist - C++ utilities for Distributed Computing


 distlib| --
:-------|---------------------------------:
Author  | [M. Massenzio](https://www.linkedin.com/in/mmassenzio)
Version | 0.5.0
Updated | 2016-08-02


[![badge](https://img.shields.io/badge/conan.io-gtest%2F1.7.0-green.svg?logo=data:image/png;base64%2CiVBORw0KGgoAAAANSUhEUgAAAA4AAAAOCAMAAAAolt3jAAAA1VBMVEUAAABhlctjlstkl8tlmMtlmMxlmcxmmcxnmsxpnMxpnM1qnc1sn85voM91oM11oc1xotB2oc56pNF6pNJ2ptJ8ptJ8ptN9ptN8p9N5qNJ9p9N9p9R8qtOBqdSAqtOAqtR%2BrNSCrNJ/rdWDrNWCsNWCsNaJs9eLs9iRvNuVvdyVv9yXwd2Zwt6axN6dxt%2Bfx%2BChyeGiyuGjyuCjyuGly%2BGlzOKmzOGozuKoz%2BKqz%2BOq0OOv1OWw1OWw1eWx1eWy1uay1%2Baz1%2Baz1%2Bez2Oe02Oe12ee22ujUGwH3AAAAAXRSTlMAQObYZgAAAAFiS0dEAIgFHUgAAAAJcEhZcwAACxMAAAsTAQCanBgAAAAHdElNRQfgBQkREyOxFIh/AAAAiklEQVQI12NgAAMbOwY4sLZ2NtQ1coVKWNvoc/Eq8XDr2wB5Ig62ekza9vaOqpK2TpoMzOxaFtwqZua2Bm4makIM7OzMAjoaCqYuxooSUqJALjs7o4yVpbowvzSUy87KqSwmxQfnsrPISyFzWeWAXCkpMaBVIC4bmCsOdgiUKwh3JojLgAQ4ZCE0AMm2D29tZwe6AAAAAElFTkSuQmCC)](http://www.conan.io/source/gtest/1.7.0/lasote/stable)

[![badge](https://img.shields.io/badge/conan.io-glog%2F0.3.4-green.svg?logo=data:image/png;base64%2CiVBORw0KGgoAAAANSUhEUgAAAA4AAAAOCAMAAAAolt3jAAAA1VBMVEUAAABhlctjlstkl8tlmMtlmMxlmcxmmcxnmsxpnMxpnM1qnc1sn85voM91oM11oc1xotB2oc56pNF6pNJ2ptJ8ptJ8ptN9ptN8p9N5qNJ9p9N9p9R8qtOBqdSAqtOAqtR%2BrNSCrNJ/rdWDrNWCsNWCsNaJs9eLs9iRvNuVvdyVv9yXwd2Zwt6axN6dxt%2Bfx%2BChyeGiyuGjyuCjyuGly%2BGlzOKmzOGozuKoz%2BKqz%2BOq0OOv1OWw1OWw1eWx1eWy1uay1%2Baz1%2Baz1%2Bez2Oe02Oe12ee22ujUGwH3AAAAAXRSTlMAQObYZgAAAAFiS0dEAIgFHUgAAAAJcEhZcwAACxMAAAsTAQCanBgAAAAHdElNRQfgBQkREyOxFIh/AAAAiklEQVQI12NgAAMbOwY4sLZ2NtQ1coVKWNvoc/Eq8XDr2wB5Ig62ekza9vaOqpK2TpoMzOxaFtwqZua2Bm4makIM7OzMAjoaCqYuxooSUqJALjs7o4yVpbowvzSUy87KqSwmxQfnsrPISyFzWeWAXCkpMaBVIC4bmCsOdgiUKwh3JojLgAQ4ZCE0AMm2D29tZwe6AAAAAElFTkSuQmCC)](http://www.conan.io/source/glog/0.3.4/dwerner/testing)

[![badge](https://img.shields.io/badge/conan.io-OpenSSL%2F1.0.2g-green.svg?logo=data:image/png;base64%2CiVBORw0KGgoAAAANSUhEUgAAAA4AAAAOCAMAAAAolt3jAAAA1VBMVEUAAABhlctjlstkl8tlmMtlmMxlmcxmmcxnmsxpnMxpnM1qnc1sn85voM91oM11oc1xotB2oc56pNF6pNJ2ptJ8ptJ8ptN9ptN8p9N5qNJ9p9N9p9R8qtOBqdSAqtOAqtR%2BrNSCrNJ/rdWDrNWCsNWCsNaJs9eLs9iRvNuVvdyVv9yXwd2Zwt6axN6dxt%2Bfx%2BChyeGiyuGjyuCjyuGly%2BGlzOKmzOGozuKoz%2BKqz%2BOq0OOv1OWw1OWw1eWx1eWy1uay1%2Baz1%2Baz1%2Bez2Oe02Oe12ee22ujUGwH3AAAAAXRSTlMAQObYZgAAAAFiS0dEAIgFHUgAAAAJcEhZcwAACxMAAAsTAQCanBgAAAAHdElNRQfgBQkREyOxFIh/AAAAiklEQVQI12NgAAMbOwY4sLZ2NtQ1coVKWNvoc/Eq8XDr2wB5Ig62ekza9vaOqpK2TpoMzOxaFtwqZua2Bm4makIM7OzMAjoaCqYuxooSUqJALjs7o4yVpbowvzSUy87KqSwmxQfnsrPISyFzWeWAXCkpMaBVIC4bmCsOdgiUKwh3JojLgAQ4ZCE0AMm2D29tZwe6AAAAAElFTkSuQmCC)](http://www.conan.io/source/OpenSSL/1.0.2g/lasote/stable)

[![badge](https://img.shields.io/badge/conan.io-cryptopp%2F5.6.3-green.svg?logo=data:image/png;base64%2CiVBORw0KGgoAAAANSUhEUgAAAA4AAAAOCAMAAAAolt3jAAAA1VBMVEUAAABhlctjlstkl8tlmMtlmMxlmcxmmcxnmsxpnMxpnM1qnc1sn85voM91oM11oc1xotB2oc56pNF6pNJ2ptJ8ptJ8ptN9ptN8p9N5qNJ9p9N9p9R8qtOBqdSAqtOAqtR%2BrNSCrNJ/rdWDrNWCsNWCsNaJs9eLs9iRvNuVvdyVv9yXwd2Zwt6axN6dxt%2Bfx%2BChyeGiyuGjyuCjyuGly%2BGlzOKmzOGozuKoz%2BKqz%2BOq0OOv1OWw1OWw1eWx1eWy1uay1%2Baz1%2Baz1%2Bez2Oe02Oe12ee22ujUGwH3AAAAAXRSTlMAQObYZgAAAAFiS0dEAIgFHUgAAAAJcEhZcwAACxMAAAsTAQCanBgAAAAHdElNRQfgBQkREyOxFIh/AAAAiklEQVQI12NgAAMbOwY4sLZ2NtQ1coVKWNvoc/Eq8XDr2wB5Ig62ekza9vaOqpK2TpoMzOxaFtwqZua2Bm4makIM7OzMAjoaCqYuxooSUqJALjs7o4yVpbowvzSUy87KqSwmxQfnsrPISyFzWeWAXCkpMaBVIC4bmCsOdgiUKwh3JojLgAQ4ZCE0AMm2D29tZwe6AAAAAElFTkSuQmCC)](http://www.conan.io/source/cryptopp/5.6.3/riebl/testing)


# Install & Build

## Conan package

See [http://conan.io]() for more information.

TO build the project, you need to first donwload/build the necessary binary dependencies, as
listed in `conanfile.text`:

```
[requires]
glog/0.3.4@dwerner/testing
OpenSSL/1.0.2g@lasote/stable
cryptopp/5.6.3@riebl/testing
zmqcpp/4.1.1@memsharded/stable
```

This is done as follows:

```bash
$ sudo -H pip install -U conan
$ mkdir .conan && cd .conan
$ conan install .. -s compiler=clang -s compiler.version=3.6 \
    -s compiler.libcxx=libstdc++ --build=missing
```

After the dependencies are built, you can see information about them using `conan info ..`
(the commands above assume that `clang` is configured on the `PATH` and that you have
version 3.6 installed).

See also `CMakeLists.txt` for the changes necessary to add Conan's builds to the targets.

### Google Tests

The `gtest` package in Conan is broken, so we need to manually donwload, build and install `gtest`.
This is pretty trivial, but you will need to add include files (`gtest/...`) and libraries 
(`libgtest.a` and `libgtest_main.a`) in the following directories, respectively:

    ${INSTALL_DIR}/include
    ${INSTALL_DIR}/lib
    
and then define the `INSTALL_DIR` variable when invoking `cmake` via `-DINSTALL_DIR` (see next 
section).

## Build & testing

To build the project, it is the usual `cmake` routine:

    $ mkdir build && cd build
    $ cmake -DINSTALL_DIR=/path/to/install/local \
            -DCMAKE_CXX_COMPILER=/usr/local/bin/clang++ \
            -DCOMMON_UTILS_DIR=/path/to/commons.cmake ..
    $ cmake --build .

Finally, to run the tests:

    ./tests/bin/test_brick

See also the other binaries in the `build/bin` folder for more options.

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
 * [Unreliable Distributed Failure Detectors for Reliable Systems](unreliable failure detectors); and
 * [A Gossip-Style Failure Detection Service](gossip failure)

 ### PING server

 This is based on [ZeroMQ C++ bindings](http://api.zeromq.org/2-1:zmq-cpp).````


[SWIM]: https://www.dropbox.com/s/hi5ft7y1o0gtm53/swim-Gossip-Protocol.pdf
[unreliable failure detectors]: https://www.dropbox.com/s/ad4l8uk5g04qz1w/UnreliableFailureDetectors.pdf
[gossip failure]: https://www.dropbox.com/s/h7owyzwobtb7nvt/GossipFailureDetection.pdf
