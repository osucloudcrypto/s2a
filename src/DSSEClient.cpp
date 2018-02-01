#include <iostream>
#include <sstream>
#include <zmq.hpp>

#include "DSSE.h"
#include "dsse.pb.h"

namespace DSSE {

bool send_message(zmq::socket_t &sock, msg::Request &msg);
template<class T> bool recv_response(zmq::socket_t &sock, T &msg);

bool Client::Connect(std::string hostname, int port)
{
	std::ostringstream buf;
	buf << "tcp://" << hostname << ":" << port;

	std::string addr = buf.str();

	// TODO catch exceptions
	this->sock.connect(addr);
	this->addr = addr;
	return true;
}

// TODO: deduplicate with DSSE.cpp
const int KEYLEN = 256/8;
typedef uint8_t key_t[KEYLEN];

bool Client::Setup(std::vector<std::string> &tokens, std::map<std::string, std::vector<fileid_t>> &fileids) {
	std::vector<SetupPair> L;
	this->core.SetupClient(tokens, fileids, L);

	msg::Request req;
	msg::Setup *msg = req.mutable_setup();
	for (auto &p : L) {
		msg::Setup_TokenPair *q = msg->add_l();
		q->set_counter(p.Token);
		q->set_fileid(p.FileID);
	}

	if (!send_message(this->sock, req)) {
		return false;
	}

	// receive empty response
	zmq::message_t zmsg;
	if (!this->sock.recv(&zmsg)) {
		return false;
	}

	// TODO: add setupresult message?
	/*
	msg::SetupResult result;
	if (!recv_response(this->sock, result)) {
		return false;
		return std::vector<fileid_t>(); // TODO return error
	}
	std::cout << "got response\n";
	*/
	return true;
}

std::vector<DSSE::fileid_t>
Client::Search(std::string w) {
	msg::Request req;
	msg::Search *msg = req.mutable_search();
	key_t K1, K2, K1plus, K2plus, K1minus;
	this->core.SearchClient(w,
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

bool send_message(zmq::socket_t &sock, msg::Request &msg) {
	std::string str;
	if (!msg.SerializeToString(&str)) {
		std::cerr << "CLIENT: encoding failed\n";
		return false;
	}
	// TODO: eliminate this copy
	zmq::message_t zmsg(str.size());
	memmove(zmsg.data(), str.data(), str.size());
	if (!sock.send(zmsg)) {
		std::cerr << "CLIENT: error sending message\n";
		return false;
	}
	return true;
}

template <class T> bool recv_response(zmq::socket_t &sock, T &resp) {
	zmq::message_t zmsg;
	if (!sock.recv(&zmsg)) {
		std::cerr << "CLIENT: error reading response\n";
		return false;
	}

	std::string str((char*)zmsg.data(), zmsg.size());
	if (!resp.ParseFromString(str)) {
		std::cerr << "CLIENT: error parsing response\n";
		return false;
	}

	return true;
}


} // namespace DSSE
