CXXFLAGS?=-O2 -ggdb -Wall
CXXFLAGS+=-std=c++11
DSSE_objs=DSSE.o

CXXFLAGS+=-I../third_party/libtomcrypt/src/headers
LIBS+=../third_party/libtomcrypt/libtomcrypt.a

all: server client

server: server.o $(DSSE_objs) Makefile
	g++ $(CXXFLAGS) $(LDFLAGS) -o server $< $(DSSE_objs) $(LIBS)

client: client.o $(DSSE_objs) Makefile
	g++ $(CXXFLAGS) $(LDFLAGS) -o client $< $(DSSE_objs) $(LIBS)

# add a dependency on the header file
$(DSSE_objs): DSSE.h

%.pb.cc: %.proto
	protoc --cpp_out=. $<
