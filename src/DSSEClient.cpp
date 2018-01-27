#include <iostream>

// yay sockets
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "DSSE.h"
#include "dsse.pb.h"
#include "netstring.h"

namespace DSSE {

bool send_message(FILE* sock, msg::Request &msg);
template<class T> bool recv_response(FILE* sock, T &msg);

bool Client::Connect(std::string hostname, int port)
{
	struct sockaddr_in addr;

	// Set up the server address struct
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	//TODO: resolve host

	// Set up the socket
	int fd = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (fd < 0) {
		perror("socket");
		fprintf(stderr, "CLIENT: error opening socket");
		return false;
	}

	// Connect to server
	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("connect");
		fprintf(stderr, "CLIENT: error connecting");
		return false;
	}

	sock = fdopen(fd, "r+b");

	//this->fd = fd;
	this->sock = sock;
	//this->hostname = hostname;
	//this->port = port;
	return true;
}

// TODO: deduplicate with DSSE.cpp
const int KEYLEN = 256/8;
typedef uint8_t key_t[KEYLEN];

std::vector<DSSE::fileid_t>
Client::Search(std::string w) {
	msg::Request req;
	msg::Search *msg = req.mutable_search();
	key_t K1, K2, K1plus, K2plus, K1minus;
	this->dsse.SearchClient(w,
		K1, K2, K1plus, K2plus, K1minus);
	msg->set_k1(std::string(reinterpret_cast<char*>(K1), sizeof K1));
	msg->set_k2(std::string(reinterpret_cast<char*>(K2), sizeof K2));
	msg->set_k1plus(std::string(reinterpret_cast<char*>(K1plus), sizeof K1plus));
	msg->set_k2plus(std::string(reinterpret_cast<char*>(K2plus), sizeof K2plus));
	msg->set_k1minus(std::string(reinterpret_cast<char*>(K1minus), sizeof K1minus));

	if (!send_message(this->sock, req)) {
		return std::vector<fileid_t>(); // TODO return error
	}

	msg::SearchResult result;
	if (!recv_response(this->sock, result)) {
		return std::vector<fileid_t>(); // TODO return error
	}

	std::cout << "got response\n";

	std::vector<fileid_t> ret(result.fileid().begin(), result.fileid().end());
	return ret;
}

bool send_message(FILE* sock, msg::Request &msg) {
	std::string str;
	if (!msg.SerializeToString(&str)) {
		std::cerr << "CLIENT: encoding failed\n";
		return false;
	}
	const char* cstr = str.c_str();
	if (write_netstring(sock, cstr, str.size()) < 0) {
		std::cerr << "CLIENT: error sending message\n";
		return false;
	}
	return true;
}

template <class T> bool recv_response(FILE* sock, T &resp) {
	int len;
	char* cstr = read_netstring(sock, &len);
	if (cstr == NULL) {
		std::cerr << "CLIENT: error reading response\n";
		return false;
	}

	std::string str(cstr, len);
	if (!resp.ParseFromString(str)) {
		std::cerr << "CLIENT: error parsing response\n";
		return false;
	}

	return true;
}


} // namespace DSSE
