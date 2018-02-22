#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include <sys/stat.h>
#include <sys/types.h>

#include "DSSE.h"

const int KEYSIZE = 256/8;

typedef std::map<std::string,std::string> Dmap;
typedef std::map<std::string,uint64_t> Dcountmap;

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
		//std::cerr << length << '\n';
		std::string v(length, '\0');
		in.read(&v[0], v.size());
		map[k] = v;
	}
	return true;
}

bool writeMapCount(std::string filename, Dcountmap& map) {
	std::fstream out;
	out.open(filename, std::ios::out | std::ios::binary);
	if (!out.is_open()) {
		perror("open");
		return false;
	}

	for (auto &pair : map) {
		auto&k = pair.first;
		auto&v = pair.second;
		out << k.size() << ":" << k << ',';
		out << static_cast<char>((v >> 0) & 0xff);
		out << static_cast<char>((v >> 8) & 0xff);
		out << static_cast<char>((v >> 16) & 0xff);
		out << static_cast<char>((v >> 24) & 0xff);
		out << static_cast<char>((v >> 32) & 0xff);
		out << static_cast<char>((v >> 40) & 0xff);
		out << static_cast<char>((v >> 48) & 0xff);
		out << static_cast<char>((v >> 56) & 0xff);
		out << '.';
	}

	out.flush();
	return true;
}

bool readMapCount(std::string filename, Dcountmap &map) {
	std::fstream in;
	in.open(filename);
	if (!in.is_open()) {
		perror("open");
		return false;
	}

	for (;;) {
		size_t size;
		char c0, c1, c2;
		in >> size >> c0;
		if (!in) {
			break;
		}
		std::string token(size, '\0');
		in.read(&token[0], size);
		in >> c1;

		if (!in) {
			break;
		}

		char b0, b1, b2, b3, b4, b5, b6, b7;
		in.get(b0);
		in.get(b1);
		in.get(b2);
		in.get(b3);
		in.get(b4);
		in.get(b5);
		in.get(b6);
		in.get(b7);
		in.get(c2);

		if (c0 != ':' || c1 != ',' || c2 != '.') {
			std::cerr << "invalid syntax in Dcount\n";
			std::cerr << c0 << c1 << c2;
			break;
		}

		if (!in) {
			break;
		}

		uint64_t v = (static_cast<uint64_t>(b0)) +
		             (static_cast<uint64_t>(b1) << 8) +
		             (static_cast<uint64_t>(b2) << 16) +
		             (static_cast<uint64_t>(b3) << 24) +
		             (static_cast<uint64_t>(b4) << 32) +
		             (static_cast<uint64_t>(b5) << 40) +
		             (static_cast<uint64_t>(b6) << 48) +
		             (static_cast<uint64_t>(b7) << 56);
		map[token] = v;
	}
	if (in.bad() || in.fail()) {
		perror("read");
		return false;
	}
	return true;
}

bool writeBytes(std::string filename, const uint8_t* bytes, size_t size) {
	std::fstream out;
	out.open(filename, std::ios::out | std::ios::binary);
	if (!out) {
		perror("open");
		return false;
	}

	out.write(reinterpret_cast<const char*>(bytes), size);
	if (!out) {
		perror("write");
		return false;
	}
	return true;
}

bool readBytes(std::string filename, uint8_t* bytes, size_t size) {
	std::fstream in;
	in.open(filename);
	if (!in) {
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

	if (!writeBytes(base+"/key", core.key, KEYSIZE)) { std::cerr<<"uhoh\n"; return false; }
	if (!writeBytes(base+"/kplus", core.kplus, KEYSIZE)) { return false; }
	if (!writeBytes(base+"/kminus", core.kminus, KEYSIZE)) { return false; }
	if (!writeMapCount(base+"/Dcount", core.Dcount)) { return false; }
	return true;
}

bool SaveServerToStorage(DSSE::Core &core, std::string base) {
	if (mkdir(base.c_str(), 0777) < 0) {
		if (errno != EEXIST) {
			perror("mkdir");
			return false;
		}
	}

	if (!writeMap(base+"/D", core.D)) { return false; }
	if (!writeMap(base+"/Dplus", core.Dplus)) { return false; }
	return true;
}

bool LoadClientFromStorage(DSSE::Core &core, std::string base) {
	if (!readBytes(base+"/key", core.key, KEYSIZE)) { return false; }
	if (!readBytes(base+"/kplus", core.kplus, KEYSIZE)) { return false; }
	if (!readBytes(base+"/kminus", core.kminus, KEYSIZE)) { return false; }
	if (!readMapCount(base+"/Dcount", core.Dcount)) { return false; }
	return true;
}

bool LoadServerFromStorage(DSSE::Core &core, std::string base) {
	if (!readMap(base+"/D", core.D)) { return false; }
	if (!readMap(base+"/Dplus", core.Dplus)) { return false; }
	return true;
}



} // namespace DSSE


/*
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
*/
