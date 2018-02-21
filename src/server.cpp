/* main entry point for the server binary */

#include <iostream>
#include <stdarg.h>

#include "DSSE.h"
#include "tomcrypt.h"

int main(int argc, char* argv[]) {
	// initialize tomcrypt
	register_hash(&sha256_desc);
	register_cipher(&aes_desc);

	DSSE::Server server;
	// TODO: init server state here
	if (argc > 1){
		int temp = atoi(argv[1]);
		server.ListenAndServe("", temp);		
	}
	else
		server.ListenAndServe("", DSSE::DefaultPort);
}

