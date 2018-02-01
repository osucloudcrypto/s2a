/* main entry point for the DSSE command-line client */

#include <cstring>
#include <cstdint>
#include <iostream>

#include "DSSE.h"
#include "tomcrypt.h"

int main() {
	// initialize tomcrypt
	register_hash(&sha256_desc);
	register_cipher(&aes_desc);

	DSSE::Core core;
	std::vector<std::string> tokens = {
		"this",
		"is",
		"a",
		"test"
	};

	std::map<std::string,std::vector<DSSE::fileid_t>> fidmap = {
		{"this", {1}},
		{"is", {1}},
		{"a", {1}},
		{"test", {1}}
	};
	std::vector<DSSE::SetupPair> L;
	core.SetupClient(tokens, fidmap, L);

	auto ids = core.SearchTest("this");
	//auto ids = fidmap.at("this");
	for (auto &id : ids) {
		std::cout << id << "\n";
	}

	DSSE::Client client;
	if (!client.Connect("localhost", DSSE::DefaultPort)) {
		std::cerr << "error connecting\n";
		return 1;
	}

	client.Setup(tokens, fidmap);
	std::cout << "setup finished\n";

	ids = client.Search("test");
	for (auto &id : ids) {
		std::cout << id << "\n";
	}

	client.Disconnect();

	return 0;
}
