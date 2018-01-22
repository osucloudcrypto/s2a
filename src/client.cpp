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

	std::string cmd = "search";
	DSSE::Core dsse;
	dsse.Init();

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
	dsse.Setup(tokens, fidmap);

	auto ids = dsse.SearchTest("this");
	for (auto &id : ids) {
		std::cout << id << "\n";
	}

	return 0;
}
