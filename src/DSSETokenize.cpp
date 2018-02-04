#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>

// DSSETokenize takes a files a returns a vector containing all words in the file and the files ID
int main()  //TODO: have program pass in file to read 
{
	std::vector <std::string> tokens;           // Vector to hold the words we read in from file.
	std::string str;                            // Temp string

	std::ifstream fin("testfile.txt");          // Open file -- temporarily hardcoded
	while (fin >> str){                         // Will read until eof and add words to vector:tokens 
		if (std::find(tokens.begin(), tokens.end(), str) == tokens.end()) //will only repeat words if the word you are adding is the same as the last word in the vector
			tokens.push_back(str);                
	}
	fin.close();                                // Close file



	for (int i = 0; i < tokens.size(); ++i)
    	std::cout << tokens.at(i) << std::endl; // Print so I can see this all worked!

return 0;
}