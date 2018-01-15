#include <string>
#include <vector>


// TODO: add types for fileid and token, and encrypted fileids and tokens

/**
 * DSSE is the core searchable encryption class.
 * It impements both the client side and server side of each operation.
 */
class DSSE {
	// Setup creates an initial index from a list of tokens and a map of
	// file id => token list
	void Setup(std::vector<std::string> &tokens, std::map<std::string, std::vector<std::string> > &fileids);

	// SearchClient performs the client side of searching the index for a given keyword.
	void SearchClient(std::string w);

	// SearchServer performs the server side of searching the index for a given keyword.
	void SearchServer(K1, K2, K1plus, K2plus, K1minus);

	// TODO: maybe split Update into separate add/edit/delete actions

	// UpdateClient performs the client side of an update action.
	// Action should be add, del, edit-, or edit+.
	void UpdateClient(std::string action, std::string id, std::vector<std::string> words);

	// UpdateServer performs the server side of an update action.
	void UpdateServer(std::string action, std::vector<std::string> L);
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

/**
 * DSSEServer serves DSSEClients.
 * It implements the network layer on top of DSSE.
 */
class DSSEServer {
	void Init();
	void HandleMessage(Message ); // i guess
}
