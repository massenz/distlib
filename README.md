# libdist - C++ utilities for Distributed Computing


 Project  |  libdist
:---      |  ---:
Author    | [M. Massenzio](https://bitbucket.org/marco)
Release   | 0.11.0
Updated   | 2020-02-26



# Install & Build

## Conan packages

To build the project, you need to first donwload/build the necessary binary dependencies, as
listed in `conanfile.text`.

This is done as follows:

```bash
sudo -H pip install -U conan
mkdir .conan && cd .conan
conan install .. -s compiler=clang -s compiler.version=4.0 \
    -s compiler.libcxx=libstdc++11 --build=missing
```

__note__
> I've found Conan not to work terribly well inside virtualenvs - but
if you can make it work there, that'd be the preferred way.

After the dependencies are built, you can see information about them using `conan info ..`
(the commands above assume that `clang` is configured on the `PATH` and that you have
version 3.6 installed).

See also `CMakeLists.txt` for the changes necessary to add Conan's builds to the targets.

We use the following packages in this project:

- [cryptopp 5.6.3](http://www.conan.io/source/cryptopp/5.6.3/riebl/testing)
- [glog 0.3.4](http://www.conan.io/source/glog/0.3.4/dwerner/testing)
- [gtest 1.8.0](http://www.conan.io/source/gtest/1.8.0/lasote/stable)
- [OpenSSL 1.0.2j](http://www.conan.io/source/OpenSSL/1.0.2j/lasote/stable)


See [conan.io](http://conan.io) for more information.

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

# Projects

## API Documentations

All the classes are documented using [Doxygen](#).
`TODO: this needs fixing, post moving out of GitHub Pages`

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
