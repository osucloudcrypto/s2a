#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include <sys/stat.h>
#include <sys/types.h>

#include "DSSE.h"

const int KEYSIZE = 256/8;

typedef std::map<std::string,std::string> Dmap;

namespace DSSE {

bool writeMap(std::string filename, Dmap& map) {
	std::fstream out;
	out.open(filename, std::ios::out | std::ios::binary);
	if (!out.is_open()) {
		perror("open");
		return false;
	}

	for (auto &pair : map) {
		auto&k = pair.first;
		auto&v = pair.second;
		if (k.size() == KEYSIZE) {
			out << k;
			out << v.size() << ','; // TODO: encode as bytes
			out << v;
		}
	}

	out.flush();

	return true;
}

bool readMap(std::string filename, Dmap &map) {
	std::fstream in;
	in.open(filename);
	if (!in.is_open()) {
		perror("open");
		return false;
	}

	for (;;) {
		std::string k(KEYSIZE, '\0');
		in.read(&k[0], k.size());
		if (in.eof()) {
			break;
		}
		size_t length;
		char comma;
		in >> length >> comma;
		std::cerr << length << '\n';
		std::string v(length, '\0');
		in.read(&v[0], v.size());
		map[k] = v;
	}
	return true;
}

bool writeBytes(std::string filename, const uint8_t* bytes, size_t size) {
	std::fstream out;
	out.open(filename);
	if (!out.is_open()) {
		perror("open");
		return false;
	}

	out.write(reinterpret_cast<const char*>(bytes), size);
	return out.good() && static_cast<size_t>(out.gcount()) == size;
}

bool readBytes(std::string filename, uint8_t* bytes, size_t size) {
	std::fstream in;
	in.open(filename, std::ios::out | std::ios::binary);
	if (!in.is_open()) {
		perror("open");
		return false;
	}
	in.read(reinterpret_cast<char*>(bytes), size);
	return in.good() && static_cast<size_t>(in.gcount()) == size;
}


bool SaveClientToStorage(DSSE::Core &core, std::string base) {
	if (mkdir(base.c_str(), 0777) < 0) {
		if (errno != EEXIST) {
			perror("mkdir");
			return false;
		}
	}

	writeBytes(base+"/key", core.key, KEYSIZE);
	writeBytes(base+"/kplus", core.kplus, KEYSIZE);
	writeBytes(base+"/kminus", core.kminus, KEYSIZE);
	//writeMap(base+"/D", core.D);
	//writeMap(base+"/Dplus", core.Dplus);
	return true;
}



} // namespace DSSE


void save() {
	Dmap m;
	m["AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"] = "aaa";
	m["BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"] = "bbbbb";
	DSSE::writeMap("test.bin", m);
	std:: cerr << "saved\n";
}

void load() {
	Dmap m;
	if (!DSSE::readMap("test.bin",m)) {
		exit(1);
	}
	for (auto p : m) {
		std::cerr << p.first << "=" << p.second << "\n";
	}
}

int main() {
	save();
	load();
}
