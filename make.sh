# This script builds the third-party libs and then builds our project

cd "$(dirname "$0")" || exit 1
top=$(pwd)
prefix="$top"/third_party

echo "building tomcrypt"
(cd third_party/libtomcrypt && make -j 12)

echo "building protobuf"
(cd third_party/protobuf && ./configure --prefix="$prefix" && make -j 12 && make install)

echo "building zeromq"
(cd third_party/zeromq && ./configure --prefix="$prefix" && make -j 12 && make install)

echo "\"building\" cppzmq"
cp -f third_party/cppzmq/zmq.hpp "$prefix/include/zmq.hpp"

echo "building s2a"
(cd src && make)
