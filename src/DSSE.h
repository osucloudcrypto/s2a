#pragma once
#include <stdio.h>
#include <cstdint>
#include <string>
#include <vector>
#include <map> // TODO: sparsehash?

#include <zmq.hpp>


namespace DSSE {

const int DefaultPort = 24992;

// TODO: add types for fileid and token, and encrypted fileids and tokens

typedef uint64_t fileid_t;

struct SetupPair {
	std::string Token;
	std::string FileID;
};

// pair of values returned by AddClient
struct AddPair {
	std::string Token;
	std::string FileID;
};

/**
 * DSSE::Core is the core searchable encryption class.
 * It impements all the cryptography for  both the client side and server side
 * of each operation.
 */
class Core {
public:
	Core();

	// TODO: need methods to:
	//     * create a new DSSE from scratch
	//     * create a DSSE based on some stored state
	// TODO: actually, i think we need a separate persitence layer

	// Setup creates an initial index from a list of tokens and a map of
	// file id => token list
	void SetupClient(
		std::vector<std::string> &tokens,
		std::map<std::string, std::vector<fileid_t> > &fileids,
		// Output
		std::vector<SetupPair> &L
	);

	void SetupServer(
		std::vector<SetupPair> &L
	);

	// SearchClient performs the client side of searching the index for a given keyword.
	// It returns a bunch of keys to send to the server.
	void SearchClient(
		// Input
		std::string w,
		// Output
		uint8_t *K1, uint8_t *K2,
		uint8_t *K1plus, uint8_t *K2plus,
		uint8_t *K1minus
	);

	// SearchServer performs the server side of searching the index for a given keyword.
	std::vector<fileid_t> SearchServer(uint8_t K1[], uint8_t K2[], uint8_t K1plus[], uint8_t K2plus[], uint8_t K1minus[]);

	// a method for testing which combines SearchClient and SearchServer
	std::vector<fileid_t> SearchTest(std::string w);

	// TODO: maybe split Update into separate add/edit/delete actions

	// Add adds keywords to a file.
	// AddClient performs the client side of the add action.
	void AddClient(
		// Input
		fileid_t id, std::vector<std::string> words,
		// Output
		std::vector<AddPair> &L
	);

	// UpdateServer performs the server side of an add action.
	void AddServer(std::vector<AddPair> L);

	// AddClient2 performs the 2nd client half of Add action
	void AddClient2(
		std::vector<std::string> r,
		std::vector<std::string> W_in_order_of_Lrev
	);

	// Delete removes keywords from a file.
	// DeletClient performs the client side of a delete action.
	// Action should be add, del, edit-, or edit+.
	void DeleteServer(std::string action, std::string id, std::vector<std::string> words);

	// Delete removes keywords from a file.
	// DeleteServer performs the server side of a delete action.
	void UpdateServer(std::string action, std::vector<std::string> L);
private:
	// Client state
	uint8_t* key; // The master key. Only used by the client
	uint8_t* kplus; // The master key for additions. Only used by the client
	uint8_t* kminus; // The master key for deletions. Only used by the client
	std::map<std::string, uint64_t> Dcount;

	// Server state;
	std::map<std::string, std::string> D; // mac'd token id -> encrypted file id
	std::map<std::string, std::string> Dplus; // mac'd token id -> encrypted file id
};

// forward-declare message types
namespace msg {
	class Request;
	class Result;
	class Search;
	class Setup;
	class Add;
}


/**
* A DSSE::Client communicates with a DSSE::Server.
 * It implements the network layer on top of DSSE::Core.
 */
class Client {
public:
	Client() : sock(zctx, ZMQ_REQ) {}

	/**
	 * Connect initiates a connection to the DSSE server.
	 * You must call connect before calling Search, Add, or Delete.
	 */
	bool Connect(std::string hostname, int port);
	bool Disconnect() {
		this->sock.disconnect(this->addr);
		return true;
	}

	bool Setup(std::vector<std::string> &tokens, std::map<std::string, std::vector<fileid_t>> &fileids);
	std::vector<fileid_t> Search(std::string w);
	bool Add(fileid_t fileid, std::vector<std::string> words);
	bool Delete(fileid_t fileid, std::vector<std::string> words);

	// Convenience methods
	//bool AddFile(std::string filename, std::string contents)
	//bool UpdateFile(std::string filename, std::string contents)
	//bool DeleteFile(std::string filename)

private:
	Core core;
	std::string addr;
	zmq::context_t zctx;
	zmq::socket_t sock;

	// XXX the client should probably store a fileid => filename mapping somewhere
};

/**
 * DSSE::Server serves DSSE::Clients.
 * It implements the network layer on top of DSSE::Core.
 */
class Server {
public:
	Server() : sock(zctx, ZMQ_REP) {}
	void ListenAndServe(std::string hostname, int port);
	void HandleMessage(const msg::Request* req);

private:
	void HandleSetup(const msg::Setup& setup);
	void HandleSearch(const msg::Search& search);
	void HandleAdd(const msg::Add &add);

	Core core;
	std::string addr;
	zmq::context_t zctx;
	zmq::socket_t sock;
};

} // namespace DSSE
