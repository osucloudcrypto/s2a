
/* main entry point for the DSSE command-line client */

#include <cstring>
#include <cstdint>
#include <iostream>
#include <unistd.h> // for getopt

#include "DSSE.h"
#include "tomcrypt.h"

void usage() {
	std::cerr << "Usage: client [-p port] <command> [args]\n";
	std::cerr << "Commands:\n";
	std::cerr << "\tclient setup\n";
	std::cerr << "\tclient search <word>\n";
	std::cerr << "\tclient add <fileid> [words...]\n";
	std::cerr << "\tclient delete <fileid> [words...]\n";
	exit(1);
}

int main(int argc, char* argv[]) {
	// initialize tomcrypt
	register_hash(&sha256_desc);
	register_cipher(&aes_desc);

	// parse command-line arguments
	int port = DSSE::DefaultPort;
	int opt;
	while ((opt = getopt(argc, argv, "p:")) != -1) {
		switch (opt) {
		case 'p':
			port = atoi(optarg);
			break;
		default: /* '?' */
			usage();
		}
	}

	if (argc < 1) {
		usage();
	}

	// Connect to the server
	DSSE::Client client;
	if (!client.Connect("localhost", port)) {
		std::cerr << "error connecting\n";
		return 1;
	}

	std::string command = argv[1];

	char** cmdargv = argv + optind + 1;
	int    cmdargc = argc - optind - 1;

	// If the command is setup, do that
	// Otherwise, attempt to load saved client state
	if (command == "setup") {
		// TODO uh, don't hardcode this
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

		client.Setup(tokens, fidmap);
		std::cerr << "setup finished\n";
	} else {
		if (client.Load("client-state")) {
			std::cerr << "info: loaded client state\n";
		} else {
			std::cerr << "error: couldn't load client state\n";
			exit(1);
		}
	}

	// Run the given command
	if (command == "setup") {
		// handled above
	} else if (command == "search") {
		if (cmdargc < 1) {
			usage();
		}
		std::string word = cmdargv[0];

		auto ids = client.Search(word);
		for (auto &id : ids) {
			std::cout << word << ": " << id << "\n";
		}

		if (ids.size() == 0) {
			std::cerr << "no results\n";
		}
	} else if (command == "add") {
		if (cmdargc < 1) {
			usage();
		}
		int fileid = atoi(cmdargv[0]);
		std::vector<std::string> words;
		for (int i = 1; i < cmdargc; i++) {
			words.push_back(cmdargv[i]);
		}

		if (!client.Add(static_cast<uint64_t>(fileid), words)) {
			std::cerr << "error: add failed, check server log\n";
		}
	} else if (command == "delete") {
		if (cmdargc < 1) {
			usage();
		}
		int fileid = atoi(cmdargv[0]);
		std::vector<std::string> words;
		for (int i = 1; i < cmdargc; i++) {
			words.push_back(cmdargv[i]);
		}

		if (!client.Delete(static_cast<uint64_t>(fileid), words)) {
			std::cerr << "error: delete failed, check server log\n";
		}
	}

	// Finally, save the client state to disk and disconnect
	if (!client.Save("client-state")) {
		std::cerr << "error: couldn't save client state\n";
	}
	client.Disconnect();

	return 0;
}
