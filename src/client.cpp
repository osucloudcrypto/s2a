/* main entry point for the DSSE command-line client */

#include <cstring>
#include <cstdint>
#include <iostream>

#include "DSSE.h"
#include "tomcrypt.h"

int main(int argc, char* argv[]) {
	// initialize tomcrypt
	register_hash(&sha256_desc);
	register_cipher(&aes_desc);

	// Check to see if user passed in a port number
	int port = DSSE::DefaultPort;
	if (argc > 1) {
		port = atoi(argv[1]);
	}

	// Some simple test data for initializing our DB
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

	// Connect to the server
	DSSE::Client client;
	if (!client.Connect("localhost", port)) {
		std::cerr << "error connecting\n";
		return 1;
	}

	// Attempt to load saved client state,
	// or run Setup if no state found
	if (client.Load("client-state")) {
		std::cout << "loaded client state\n";
	} else {
		client.Setup(tokens, fidmap);
		std::cout << "setup finished\n";
	}

	// Simple search test
	auto ids = client.Search("test");
	for (auto &id : ids) {
		std::cout << id << "\n";
	}

	// Test adding a keyword
	ids = client.Search("balloons");
	for (auto &id : ids) {
		std::cout << "balloons: " << id << "\n";
	}
	if (!client.Add(4, std::vector<std::string>{"balloons"})) {
		std::cout << "add failed\n";
	}
	ids = client.Search("balloons");
	for (auto &id : ids) {
		std::cout << "balloons: " << id << "\n";
	}

	// Finally, save the client state to disk and disconnect
	client.Save("client-state");

	client.Disconnect();

	return 0;
}
