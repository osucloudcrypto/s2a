#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include "DSSE.h"

namespace DSSE {

// tokenize takes a files a returns a vector containing all words in the file and the files ID
bool tokenize(std::string filename, std::vector<std::string> &tokens) {
	std::ifstream fin(filename);
	if (!fin.is_open()) {
		perror("open");
		return false;
	}

	// Read until eof and add words to vector:tokens
	std::set<std::string> token_set;
	std::string str;
	while (fin >> str) {
		// Don't add duplicate words.
		token_set.insert(str);
	}
	fin.close();

	tokens = std::vector<std::string>(token_set.begin(), token_set.end());
	return true;
}

}
