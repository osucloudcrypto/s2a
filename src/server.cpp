/* main entry point for the server binary */

#include <iostream>
#include <stdarg.h>

#include "DSSE.h"
#include "tomcrypt.h"

int main() {
	// initialize tomcrypt
	register_hash(&sha256_desc);
	register_cipher(&aes_desc);

	DSSE::Server server;
	std::string saveDir = "server-state";
	server.SetSaveDir(saveDir);
	if (!server.Load()) {
		std::cerr << "warning: unable to load server state\n";
	}
	server.ListenAndServe("", DSSE::DefaultPort);
}

