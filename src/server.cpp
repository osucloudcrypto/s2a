/* main entry point for the server binary */

#include <iostream>
#include <stdarg.h>

#include "DSSE.h"

int main() {
	DSSE::Server server;
	// TODO: init server state here
	server.ListenAndServe("", DSSE::DefaultPort);
}

