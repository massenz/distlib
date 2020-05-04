# libdist - C++ utilities for Distributed Computing


Project   | libdist
:---      | ---:
Author    | [M. Massenzio](https://bitbucket.org/marco)
Release   | 0.12.0
Updated   | 2020-03-22

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

# Install & Build

## Conan packages

This project use [Conan](https://conan.io) to build dependencies; please see 
[this post](https://medium.com/swlh/converting-a-c-project-to-cmake-conan-61ba9a998cb4) 
for more details on how to use inside a CMake project.

To build the project, you need to first download/build the necessary binary dependencies, as
listed in `conanfile.txt`.

This is done as follows (we highly recommend to use [virtualenvs in Python](https://virtualenv.pypa.io/en/stable/)):

```bash
pip install -U conan
mkdir build && cd build
conan install . -s compiler=clang -s compiler.version=$VERSION \
    -s compiler.libcxx=libstdc++11 --build=missing
```


After the dependencies are built, you can see information about them using `conan info build`
(the commands above assume that `clang` is configured on the `PATH` and that you have
version `$VERSION` installed).

See also `CMakeLists.txt` for the changes necessary to add Conan's builds to the targets.

We use the following packages in this project:

- [cryptopp 5.6.3](http://www.conan.io/source/cryptopp/5.6.3/riebl/testing)
- [glog 0.3.4](http://www.conan.io/source/glog/0.3.4/dwerner/testing)
- [gtest 1.8.0](http://www.conan.io/source/gtest/1.8.0/lasote/stable)
- [OpenSSL 1.0.2j](http://www.conan.io/source/OpenSSL/1.0.2j/lasote/stable)


See [conan.io](http://conan.io) for more information.

## Build & testing

### Common utilities

The scripts in this repository take advantage of shared common utility functions
in [this common utils repository](https://bitbucket.org/marco/common-utils): clone it
somewhere, and make `$COMMON_UTILS_DIR` point to it:

```shell script
git clone git@bitbucket.org:marco/common-utils.git
export COMMON_UTILS_DIR="$(pwd)/common-utils"
```

### Build libdist

To build the project:

```shell script
$ export INSTALL_DIR=/some/path
$ ./bin/build && ./bin/test
```

or to simply run a subset of the tests with full debug logging:

    $ GLOG_v=2 ./bin/test --gtest_filter=SwimServer*

To install the generated binaries (`.so` or `.dylib` shared libraries) 
and headers so that other projects can find them:

    $ cd build && make install

See the scripts in the `bin` folder for more options.

### Run the examples

There is one example binary that shows how to use consistent hashes and build a Merkle tree:
see `src/examples/merkle_demo.cpp`.

After [building](#build-libdist) the project, you can run it with:

    ./build/bin/merkle_demo  "some string to hash" 8


# Projects

## API Documentations

All the classes are documented using [Doxygen](http://www.doxygen.nl/), just run:

    $ doxygen

from the main project directory and all apidocs will be generated in `docs/apidocs/html`.

## Consistent Hashing

See the [Consistent Hash paper](chash) for more details.

The code implementation here is a simple example of how to implement a set of `buckets` so that
nodes in a distributed systems could use the consistent hashing algorithm to allow nodes to
join/leave the ring, without causing massive reshuffles of the partitioned data.

A `View` is then a collection of `Buckets`, which define how the unity circle is divided, via the
`consistent_hashing()` method and every partitioned item is allocated to a (named) `Bucket`: see
the [tests](tests/test_view.cpp) for an example of adding/removing buckets and how this only
causes a fraction of the items to be re-shuffled.


## Merkle Trees

A Merkle Tree is a tree structure where the individual nodes’ hashes are computed and stored in the parent node, so that the hash stored in the root node is the only value that is required to later validate the contents of the tree (stored in the leaves).

Merkle trees are used in the Blockchain and are described in greater detail in "[Mastering bitcoin](bitcoin)" (Chapter 7, “Merkle Trees”):

See [this post](https://codetrips.com/2016/06/19/implementing-a-merkle-tree-in-c/) for more details.
 

[bitcoin]: https://amzn.to/32B4jYE
[chash]: http://www.cs.princeton.edu/courses/archive/fall07/cos518/papers/chash.pdf
