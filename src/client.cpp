
/* main entry point for the DSSE command-line client */

#include <cstring>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <unistd.h> // for getopt

#include "DSSE.h"
#include "tomcrypt.h"

void usage() {
	std::cerr << "Usage: client [-p port] <command> [args]\n";
	std::cerr << "Commands:\n";
	std::cerr << "\tclient setup [files...]\n";
	std::cerr << "\tclient search <word>\n";
	std::cerr << "\tclient add <fileid> [words...]\n";
	std::cerr << "\tclient addfile <filename>\n";
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

	std::string command = argv[optind];

	char** cmdargv = argv + optind + 1;
	int    cmdargc = argc - optind - 1;

	// Connect to the server
	DSSE::Client client;
	if (!client.Connect("localhost", port)) {
		std::cerr << "error connecting\n";
		return 1;
	}

	// If the command is setup, do that
	// Otherwise, attempt to load saved client state
	if (command == "setup") {
		// if we weren't given any files,
		// seed with some test data
		if (cmdargc == 0) {
			std::vector<std::string> all_tokens;
			std::map<std::string,std::vector<DSSE::fileid_t>> fidmap;
			all_tokens.push_back("this");
			all_tokens.push_back("is");
			all_tokens.push_back("a");
			all_tokens.push_back("test");
			fidmap["this"].push_back(1);
			fidmap["is"].push_back(1);
			fidmap["a"].push_back(1);
			fidmap["test"].push_back(1);
			client.Setup(all_tokens, fidmap);
		} else {
			std::vector<std::string> filenames;
			for (int i = 0; i < cmdargc; i++) {
				std::string filename = cmdargv[i];
				filenames.push_back(filename);
			}
			client.SetupFiles(filenames);
		}

		std::cerr << "setup finished\n";
	} else if (command == "setuplist") {
		if (cmdargc != 1) {
			std::cerr << "error: please supply a filename\n";
			exit(1);
		}
		std::ifstream manifest;
		manifest.open(cmdargv[0]);
		if (!manifest) {
			std::cerr << "error: couldn't open file";
			exit(1);
		}

		std::vector<std::string> filenames;
		std::string filename;
		while (getline(manifest, filename)) {
			filenames.push_back(filename);
		}
		client.SetupFiles(filenames);
		std::cerr << "setup finished; added " << filenames.size() << " files\n";
	} else {
		if (client.Load("client-state")) {
			std::cerr << "info: loaded client state\n";
		} else {
			std::cerr << "error: couldn't load client state\n";
			exit(1);
		}
	}

	// Run the given command
	if (command == "setup" || command == "setuplist") {
		// handled above
	} else if (command == "search") {
		if (cmdargc < 1) {
			usage();
		}
		std::string word = cmdargv[0];

		auto ids = client.Search(word);
		for (auto &id : ids) {
			std::string filename = client.Filename(id);
			if (filename.empty()) {
				std::cout << word << ": " << id << "\n";
			} else {
				std::cout << word << ": " << filename << "\n";
			}
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
	} else if (command == "addfile") {
		if (cmdargc < 1) {
			usage();
		}
		std::string filename = cmdargv[0];
		if (!client.AddFileByName(filename)) {
			std::cerr << "error: add failed, check server log\n";
			//XXX exit?
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
	} else {
		std::cerr << "error: unknown command " << command << "\n";
		exit(1);
	}

	// Finally, save the client state to disk and disconnect
	if (!client.Save("client-state")) {
		std::cerr << "error: couldn't save client state\n";
	}
	client.Disconnect();

	return 0;
}
