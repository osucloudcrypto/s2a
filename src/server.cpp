/* main entry point for the server binary */

#include <iostream>
#include <stdarg.h>

#include "DSSE.h"
#include "tomcrypt.h"

int main(int argc, char* argv[]) {
	// initialize tomcrypt
	register_hash(&sha256_desc);
	register_cipher(&aes_desc);

	std::string saveDir = "server-state";
	int port = DSSE::DefaultPort;

	if (argc > 1) {
		port = atoi(argv[1]);
	}

	if (argc > 2) {
		saveDir = argv[2];
	}

	DSSE::Server server;
	server.SetSaveDir(saveDir);
	if (!server.Load()) {
		std::cerr << "warning: unable to load server state\n";
	}

	server.ListenAndServe("", port);
}

