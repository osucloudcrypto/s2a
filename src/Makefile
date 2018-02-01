CXXFLAGS?=-O2 -ggdb -Wall
CXXFLAGS+=-std=c++11
CFLAGS?=-O2 -ggdb -Wall
CFLAGS+=-std=c99

DSSE_objs=DSSE.o DSSEClient.o DSSEServer.o dsse.pb.o netstring.o


CXXFLAGS+=-I../third_party/libtomcrypt/src/headers
LIBS+=../third_party/libtomcrypt/libtomcrypt.a

CXXFLAGS+=-I../third_party/include
LIBS+=../third_party/lib/libprotobuf.a

CXXFLAGS+=-I../third_party/include
LIBS+=../third_party/lib/libzmq.a

all: server client
protobuf: dsse.pb.cc

server: server.o $(DSSE_objs) Makefile
	g++ $(CXXFLAGS) $(LDFLAGS) -o server $< $(DSSE_objs) $(LIBS)

client: client.o $(DSSE_objs) Makefile
	g++ $(CXXFLAGS) $(LDFLAGS) -o client $< $(DSSE_objs) $(LIBS)

# add a dependency on the header files
client.o server.o $(DSSE_objs): DSSE.h dsse.pb.h

%.pb.cc: %.proto Makefile
	../third_party/bin/protoc --cpp_out=. $<

%.pb.h: %.pb.cc
	@:
