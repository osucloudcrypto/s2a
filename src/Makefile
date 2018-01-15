CFLAGS?=-O2 -ggdb -Wall

all: server client

server: server.cpp
	g++ $(CFLAGS) $(LDFLAGS) -o server server.cpp

client: client.cpp
	g++ $(CFLAGS) $(LDFLAGS) -o client client.cpp
