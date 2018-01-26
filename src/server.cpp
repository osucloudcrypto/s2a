/* main entry point for the server binary */

#include <iostream>
#include <stdarg.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "DSSE.h"
#include "dsse.pb.h"
#include "netstring.h"

void handle(int fd);

DSSE::Server server;

void die(const char* format, ...) {
	va_list va;
	va_start(va, format);
	vfprintf(stderr, format, va);
	va_end(va);
	fprintf(stderr, "\n");
	exit(1);
}


int main() {
	// TODO: init server state here

	// TODO: move all this gunk to the Server class

	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in listenaddr = {0};

	int port = 24992;
	int maxconn = 5;
	bool reuseport = true;

	// Set up the address struct for this process (the server)
	listenaddr.sin_family = AF_INET;
	listenaddr.sin_port = htons(port);
	listenaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any address is allowed for connection to this process

	// Set up the socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		perror("socket");
		die("error opening socket");
	}

	// Attempt to set the reuse port option
	// This allows us to bind to the socket
	// even if another process is using it
	if (reuseport) {
		int optval = 1;
		if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval) < 0) {
			// print an warning if setting REUSEPORT fails,
			// but continue since it isn't critical
			perror("setsockopt");
		}
	}

	// Enable the socket to begin listening
	if (bind(listenfd, (struct sockaddr *)&listenaddr, sizeof(listenaddr)) < 0) {
		perror("bind");
		die("error on binding");
	}

	std::cout << "Listening on port " << port << "\n";

	// Flip the socket on - it can now receive up to maxconn connections
	listen(listenfd, maxconn);

	for (;;) {
		int fd;

		// Accept a connection, blocking if one is not available until one connects
		fd = accept(listenfd, NULL, 0);
		if (fd < 0) {
			perror("accept");
			continue;
		}

		// Handle the connection.
		// We don't fork off a handler process because DSSE
		// isn't built to handle concurrent access (yet).
		handle(fd);

		// Close the socket
		if (close(fd) < 0) {
			perror("close");
		}
	}

	close(listenfd); // Close the listening socket

	return 0;
}

template <class T> bool send_message(FILE* sock, T &msg);

void handle(int fd) {
	// read message
	FILE* sock = fdopen(fd, "r+b");
	if (sock == NULL) {
		perror("fdopen");
		return;
	}

	int len;
	char* cstr = read_netstring(sock, &len);
	if (cstr == NULL) {
		fprintf(stderr, "SERVER: error reading message from socket\n");
		return;
	}

	std::string str(cstr, len);

	DSSE::msg::Request req;
	if (!req.ParseFromString(str)) {
		fprintf(stderr, "SERVER: error parsing message\n");
		return;
	}

	server.HandleMessage(&req);

	DSSE::msg::SearchResult result;
	result.add_fileid(42);
	if (!send_message(sock, result)) {
		fprintf(stderr, "SERVER: error sending result\n");
		return;
	}

	if (fflush(sock) < 0) {
		perror("fflush");
	}
}

template <class T> bool send_message(FILE* sock, T &msg) {
	std::string str;
	if (!msg.SerializeToString(&str)) {
		std::cerr << "SERVER: encoding failed\n";
		return false;
	}
	const char* cstr = str.c_str();
	if (write_netstring(sock, cstr, str.size()) < 0) {
		std::cerr << "SERVER: error sending message\n";
		return false;
	}
	return true;
}
