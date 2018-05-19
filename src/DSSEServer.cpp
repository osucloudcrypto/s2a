#include <iostream>
#include <sstream>
#include <zmq.hpp>

#include "DSSE.h"
#include "dsse.pb.h"

namespace DSSE {

template <class T> bool send_message(zmq::socket_t& sock, T &msg);

void Server::HandleMessage(const msg::Request* req) {
	if (req->has_setup()) {
		std::cout << "Got a setup request\n";
		this->HandleSetup(*req);
	} else if (req->has_search()) {
		std::cout << "Got a search request\n";
		this->HandleSearch(req->search());
	} else if (req->has_add()) {
		//std::cout << "Got an add request\n";
		this->HandleAdd(req->add());
	} else if (req->has_delete_()) {
		std::cout << "Got a delete request\n";
		this->HandleDelete(req->delete_());
	} else {
		std::cerr << "SERVER got unknown message type\n";
	}
}

void Server::HandleSetup(const msg::Request &req) {
	const msg::Setup &setup = req.setup();
	std::vector<SetupPair> L;
	std::vector<std::string> M;
	for (auto &p : setup.l()) {
		L.push_back(SetupPair{p.counter(), p.fileid()});
	}
	for (auto &block : setup.m()) {
		M.push_back(block);
	}
	int version = req.version();
	this->core.SetupServer(version, L, M);
	// send back and empty reply
	// TODO: send back an "OK" message?
	zmq::message_t response(0);
	this->sock.send(response);
}

void Server::HandleSearch(const msg::Search &search) {
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

void Server::HandleAdd(const msg::Add &add) {
	std::vector<AddPair> L;
	for (auto &p : add.l()) {
		L.push_back(AddPair{
			p.token(),
			p.fileid(),
			p.revid(),
		});
	}

	std::vector<unsigned char> r;
	this->core.AddServer(L, r);

	msg::Result result;
	msg::AddResult *addresult = result.mutable_add();
	for (unsigned char x : r) {
		addresult->add_r(x != 0);
	}

	if (!send_message(this->sock, result)) {
		fprintf(stderr, "SERVER: error sending result\n");
	}
}

void Server::HandleDelete(const msg::Delete &msg) {
	std::vector<std::string> L;
	for (auto &revid : msg.l()) {
		L.push_back(revid);
	}

	this->core.DeleteServer(L);

	msg::Result result;
	msg::DeleteResult *delresult = result.mutable_delete_();
	(void)delresult;

	if (!send_message(this->sock, result)) {
		fprintf(stderr, "SERVER: error sending result\n");
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

			// TODO: eliminate this copy somehow?
			std::string str((char*)zmsg.data(), zmsg.size());

			msg::Request req;
			if (!req.ParseFromString(str)) {
				// FIXME: we have to send back an response
				fprintf(stderr, "SERVER: error parsing message\n");
				continue;
			}

			this->HandleMessage(&req);

			// save state after every request
			// except search requests (because they don't modify any data)
			if (!this->saveDir.empty()) {
				if (!req.has_search()) {
					SaveServerToStorage(this->core, this->saveDir);
				}
			}
		}
	}
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
