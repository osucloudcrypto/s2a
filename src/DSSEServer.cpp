#include <iostream>
#include <sstream>
#include <zmq.hpp>

#include "DSSE.h"
#include "dsse.pb.h"

namespace DSSE {

const int KEYLEN = 256/8; // XXX

template <class T> bool send_message(zmq::socket_t& sock, T &msg);
void handle(Server* server, zmq::message_t&);

void Server::HandleMessage(const msg::Request* req) {
	if (req->has_setup()) {
		std::cout << "Got a setup request\n";
		this->HandleSetup(req->setup());
	} else if (req->has_search()) {
		std::cout << "Got a search request\n";
		this->HandleSearch(req->search());
	} else {
		std::cerr << "SERVER got unknown message type\n";
	}
}

void Server::HandleSetup(const msg::Setup &setup) {
	std::vector<SetupPair> L;
	for (auto &p : setup.l()) {
		L.push_back(SetupPair{p.counter(), p.fileid()});
	}
	this->core.SetupServer(L);
	// send back and empty reply
	// TODO: send back an "OK" message?
	zmq::message_t response(0);
	this->sock.send(response);
}

void Server::HandleSearch(const msg::Search &search) {
	typedef uint8_t key_t[256/8];
	key_t K1, K2, K1plus, K2plus, K1minus;
	if (search.k1().size() != KEYLEN) {
		fprintf(stderr, "SERVER: K1 has wrong length %zd\n", search.k1().size());
		return;
	}
	if (search.k2().size() != KEYLEN) {
		fprintf(stderr, "SERVER: K2 has wrong length %zd\n", search.k2().size());
		return;
	}
	if (search.k1plus().size() != KEYLEN) {
		fprintf(stderr, "SERVER: K1plus has wrong length %zd\n", search.k1plus().size());
		return;
	}
	if (search.k2plus().size() != KEYLEN) {
		fprintf(stderr, "SERVER: K2plus has wrong length %zd\n", search.k2plus().size());
		return;
	}
	if (search.k1minus().size() != KEYLEN) {
		fprintf(stderr, "SERVER: K1minus has wrong length %zd\n", search.k1minus().size());
		return;
	}
	memmove(K1, search.k1().data(), KEYLEN);
	memmove(K2, search.k2().data(), KEYLEN);
	memmove(K1plus, search.k1plus().data(), KEYLEN);
	memmove(K2plus, search.k2plus().data(), KEYLEN);
	memmove(K1minus, search.k1minus().data(), KEYLEN);
	auto vec = this->core.SearchServer(K1, K2, K1plus, K2plus, K1minus);
	msg::SearchResult result;
	for (auto fileid : vec) {
		result.add_fileid(fileid);
	}
	if (!send_message(this->sock, result)) {
		fprintf(stderr, "SERVER: error sending result\n");
		return;
	}
}


void Server::ListenAndServe(std::string hostname, int port) {
	std::ostringstream buf;
	if (hostname == "") {
		hostname = "*";
	}
	buf << "tcp://" << hostname << ":" << port;

	std::string addr = buf.str();

	this->sock.bind(addr);

	std::cout << "Listening on port " << port << "\n";

	for (;;) {
		zmq::message_t zmsg;
		if (this->sock.recv(&zmsg)) {
			// TODO: handle in a separate thread?
			handle(this, zmsg);
		}
	}
}

void handle(Server* server, zmq::message_t &zmsg) {
	// TODO: eliminate this copy somehow?
	std::string str((char*)zmsg.data(), zmsg.size());

	msg::Request req;
	if (!req.ParseFromString(str)) {
		// FIXME: we have to send back an response
		fprintf(stderr, "SERVER: error parsing message\n");
		return;
	}

	server->HandleMessage(&req);
}

template <class T> bool send_message(zmq::socket_t& sock, T &msg) {
	std::string str;
	if (!msg.SerializeToString(&str)) {
		std::cerr << "SERVER: encoding failed\n";
		return false;
	}
	//zmq::message_t zmsg((void*)str.data(), str.size(), nullptr);
	// TODO: Eliminate this copy
	zmq::message_t zmsg(str.size());
	memmove(zmsg.data(), str.data(), str.size());
	if (!sock.send(zmsg)) {
		std::cerr << "SERVER: error sending message\n";
		return false;
	}
	return true;
}

} // namespace DSSE
