CFLAGS?=-O2 -ggdb -Wall
DSSE_objs=DSSE.o

all: server client

server: server.o $(DSSE_objs) Makefile
	g++ $(CFLAGS) $(LDFLAGS) -o server $< $(DSSE_objs)

client: client.o $(DSSE_objs) Makefile
	g++ $(CFLAGS) $(LDFLAGS) -o client $< $(DSSE_objs)

