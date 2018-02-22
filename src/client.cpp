/* main entry point for the DSSE command-line client */

#include <cstring>
#include <cstdint>
#include <iostream>

// Used for Benchmarking
#include <chrono>
#include <stdlib.h>
#include <time.h>

// fstream used to write benchmarks to file
#include <fstream>

#include <unistd.h>


#include "DSSE.h"
#include "tomcrypt.h"


int main() {
	// initialize tomcrypt
	register_hash(&sha256_desc);
	register_cipher(&aes_desc);

	
	DSSE::Core core;
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
	std::vector<DSSE::SetupPair> L;
	core.SetupClient(tokens, fidmap, L);

	auto ids = core.SearchTest("this");
	//auto ids = fidmap.at("this");
	for (auto &id : ids) {
		std::cout << id << "\n";
	}

	DSSE::Client client;
	if (!client.Connect("localhost", DSSE::DefaultPort)) {
		std::cerr << "error connecting\n";
		return 1;
	}

	client.Setup(tokens, fidmap);
	std::cout << "setup finished\n";

	ids = client.Search("test");
	for (auto &id : ids) {
		std::cout << id << "\n";
	}

	
	ids = client.Search("balloons");
	
	
	
	
	for (auto &id : ids) {
		std::cout << "balloons: " << id << "\n";
	}
	
	
	// Creating file for add testing
	std::ofstream myfile;
	char Add_File[] = "Add_Benchmarks.txt";
	myfile.open (Add_File, std::ios_base::app);
	
	
	
	
	
	
	
	
	
	
	// Testing Add
	auto start = std::chrono::steady_clock::now();
	
	int size_of_database = 100000;
	
	// Adding 4 id's Each named Balloons
	for (int i = 0; i < size_of_database; i++)
	if (!client.Add(i, std::vector<std::string>{"balloons"})) {
		std::cout << "add failed\n";
	}
	
	
	// Generate Random Time Stamp for 
	srand (time(NULL));
	int random_replace = rand()% size_of_database;
	// Select Random Place in index to place unique item;
	std::cout<<"Random Unique Entry Placed at index: "<<random_replace<<std::endl;
	if (!client.Add(random_replace, std::vector<std::string>{"Chocolate"})) {
		std::cout << "add failed\n";
	}
	
	
	// Print out Time to add size_of_database items
	
	auto end = std::chrono::steady_clock::now();
	std::cout 	<<"\n Testing (Add: "<<size_of_database<<"): \n"
				<< "Elapsed Time: "
				<< std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()
				<< " in nanoseconds \n" << std::endl;
			
		// Push Add output to File for Benchmarking
		myfile <<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<"\r\n";
		myfile.close();		
	
	
	
	
	
	
	
	
	// Testing Search 
	char Add_File_2[] = "Search_Benchmarks.txt";
	myfile.open (Add_File_2, std::ios_base::app);
	 start = std::chrono::steady_clock::now();
	
//	ids = client.Search("balloons");

	// Unique ID
	ids = client.Search("Chocolate");
	end = std::chrono::steady_clock::now();
	std::cout 	<<"\n Testing (Search Unique: "<<size_of_database<<"): \n"
				<< "Elapsed Time: "
				<< std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()
				<< " in nanoseconds \n" << std::endl;

	// Push Search output to File for Benchmarking
		myfile <<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<"\r\n";
		myfile.close();					
				
	for (auto &id : ids) {
		//std::cout << "balloons: " << id << "\n";
	}

	// Testing Time Elapsed
	
	client.Disconnect();

	return 0;
}
