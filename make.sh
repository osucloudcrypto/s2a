# This script builds the third-party libs and then builds our project

top=$(dirname "$0")
prefix="$top"/third_party
cd "$top" || exit 1

echo "building tomcrypt"
(cd third_party/libtomcrypt && make -j 12)

echo "building protobuf"
(cd third_party/protobuf && ./configure --prefix="$prefix" && make -j 12 && make install)

echo "building zeromq"
(cd third_party/zeromq && ./configure --prefix="$prefix" && make -j 12 && make install)

echo "building s2a"
(cd src && make)
