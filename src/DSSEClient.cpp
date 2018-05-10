#include <iostream>
#include <sstream>
#include <zmq.hpp>

#include "DSSE.h"
#include "dsse.pb.h"

namespace DSSE {

typedef std::string filename_t;

bool send_message(zmq::socket_t &sock, msg::Request &msg);
template<class T> bool recv_response(zmq::socket_t &sock, T &msg);

std::string Client::Filename(fileid_t fileid)
{
	if (this->fileidMap.count(fileid) >= 0) {
		return this->fileidMap.at(fileid);
	}
	return "";
}

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

bool Client::SetupFiles(int version, std::vector<std::string> &filenames) {
	std::map<filename_t, fileid_t> filenameToFileid;
	std::map<fileid_t, filename_t> fileidToFilename;
	std::map<std::string, std::vector<fileid_t>> fileids;
	std::vector<std::string> all_tokens;

	fileid_t lastFileid = 1;
	for (auto& filename : filenames) {
		std::vector<std::string> file_tokens;
		if (tokenize(filename, file_tokens)) {
			// allocate a fileid if this is a new filename
			if (filenameToFileid.count(filename) <= 0) {
				filenameToFileid[filename] = lastFileid;
				fileidToFilename[lastFileid] = filename;
				lastFileid++;
			}
			fileid_t fileid = filenameToFileid.at(filename);
			for (auto &word : file_tokens) {
				// add word to all_tokens if this is a new token
				if (fileids.count(word) <= 0) {
					all_tokens.push_back(word);
					//std::cerr << "debug: word: "<<word<<"\n";
				}

				fileids[word].push_back(fileid);
			}
		} else {
			// error tokenizing file
		}
	}

	if (!this->Setup(version, all_tokens, fileids)) {
		return false;
	}

	this->lastFileid = lastFileid;
	this->fileidMap = std::move(fileidToFilename);

	return true;
}

bool Client::AddFileByName(std::string filename) {
	std::vector<std::string> tokens;

	if (!tokenize(filename, tokens)) {
		return false;
	}

	// TODO: check if file has already been added?

	fileid_t fileid = this->lastFileid;

	if (!this->Add(fileid, tokens)) {
		return false;
	}

	this->fileidMap[fileid] = filename;
	this->lastFileid++;
	return true;
}

bool Client::Setup(int version, std::vector<std::string> &tokens, std::map<std::string, std::vector<fileid_t>> &fileids) {
	std::vector<SetupPair> L;
	this->core.SetupClient(version, tokens, fileids, L);

	msg::Request req;
	// TODO: send version on every request, not just Setup
	req.set_version(static_cast<msg::Version>(version));

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

bool Client::Add(fileid_t fileid, std::vector<std::string> w) {
	std::vector<AddPair> L;
	std::vector<std::string> W_in_order_of_Lrev;
	this->core.AddClient(fileid, w, L, W_in_order_of_Lrev);

	msg::Request req;
	msg::Add *msg = req.mutable_add();
	for (auto &p : L) {
		auto q = msg->add_l();
		q->set_token(p.Token);
		q->set_fileid(p.FileID);
		q->set_revid(p.RevID);
	}

	if (!send_message(this->sock, req)) {
		// TODO: throw an error
		return false;
	}

	msg::Result result;
	if (!recv_response(this->sock, result)) {
		// TODO: throw an error
		return false;
	}

	std::vector<unsigned char> r;
	for (bool x : result.add().r()) {
		r.push_back(x?1:0);
	}

	this->core.AddClient2(r, W_in_order_of_Lrev);
	return true;
}

bool Client::Delete(fileid_t fileid, std::vector<std::string> words) {
	std::vector<std::string> L;
	this->core.DeleteClient(fileid, words, L);

	msg::Request req;
	msg::Delete *msg = req.mutable_delete_();
	for (auto &revid : L) {
		msg->add_l(revid);
	}

	if (!send_message(this->sock, req)) {
		// TODO: throw an error
		return false;
	}

	msg::Result result;
	if (!recv_response(this->sock, result)) {
		// TODO: throw an error
		return false;
	}
	return true;
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
