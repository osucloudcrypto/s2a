# This script builds the third-party libs and then builds our project

cd "$(dirname "$0")"
echo "building tomcrypt"
(cd third_party/libtomcrypt && make)
echo "building protobuf"
(cd third_party/protobuf && ./configure --prefix="$PWD"/.. && make -j 10 && make install)
echo "building s2a"
(cd src && make)
