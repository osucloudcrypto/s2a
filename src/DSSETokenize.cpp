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
	std::string str;
	while (fin >> str){
		// Don't add duplicate words.
		if (std::find(tokens.begin(), tokens.end(), str) == tokens.end()) {
			tokens.push_back(str);
		}
	}
	fin.close();
	return true;
}

}
