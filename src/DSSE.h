#include <cstdint>
#include <string>
#include <vector>
#include <map> // TODO: sparsehash?


// TODO: add types for fileid and token, and encrypted fileids and tokens

typedef uint64_t fileid_t;

/**
 * DSSE is the core searchable encryption class.
 * It impements both the client side and server side of each operation.
 */
class DSSE {
public:
	void Init();
	// TODO: need methods to:
	//     * create a new DSSE from scratch
	//     * create a DSSE based on some stored state

	// Setup creates an initial index from a list of tokens and a map of
	// file id => token list
	void Setup(std::vector<std::string> &tokens, std::map<std::string, std::vector<fileid_t> > &fileids);

	// SearchClient performs the client side of searching the index for a given keyword.
	void SearchClient(std::string w);

	// SearchServer performs the server side of searching the index for a given keyword.
	std::vector<fileid_t> SearchServer(uint8_t K1[], uint8_t K2[], uint8_t K1plus[], uint8_t K2plus[], uint8_t K1minus[]);

	// a method for testing which combines SearchClient and SearchServer
	std::vector<fileid_t> SearchTest(std::string w);

	// TODO: maybe split Update into separate add/edit/delete actions

	// UpdateClient performs the client side of an update action.
	// Action should be add, del, edit-, or edit+.
	void UpdateClient(std::string action, std::string id, std::vector<std::string> words);

	// UpdateServer performs the server side of an update action.
	void UpdateServer(std::string action, std::vector<std::string> L);

private:
	// Data
	uint8_t* key; // The master key. Only used by the client

	// Server state;
	std::map<std::string, std::string> D; // mac'd token id -> encrypted file id
	std::map<std::string, std::string> Dplus; // mac'd token id -> encrypted file id
};

/**
* A DSSEClient communicates with a DSSEServer.
 * It implement the network layer on top of DSSE.
 */
class DSSEClient {
	void Init();
	std::vector<std::string> Search(std::string w);
	bool Add(std::string fileid, std::string word);
	bool Delete(std::string fileid);
};

/// Some sort of message class
// TODO: maybe use protobuf?
class Message {

};

/**
 * DSSEServer serves DSSEClients.
 * It implements the network layer on top of DSSE.
 */
class DSSEServer {
	void Init();
	void HandleMessage(Message *msg); // i guess
};
