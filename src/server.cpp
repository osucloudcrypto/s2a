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
	// TODO: init server state here
	server.ListenAndServe("", DSSE::DefaultPort);
}

