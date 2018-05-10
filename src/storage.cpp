#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include <sys/stat.h>
#include <sys/types.h>

#include "DSSE.h"

typedef std::map<std::string,std::string> Dmap;
typedef std::map<std::string,uint64_t> Dcountmap;
typedef std::set<std::string> Revlist;

namespace DSSE {

// writeString writes a std::string in netstring format
void writeString(std::ofstream& out, const std::string& s) {
	out << s.size() << ":" << s << ',';
}

// readString reads a netstring-formatted string
// returns true if successful or on eof; check in.eof() to differentiate
// returns false otherwise
bool readString(std::ifstream& in, std::string& s) {
	size_t size;
	char c0, c1;
	in >> size;
	if (in.eof()) {
		s = "";
		return true;
	}
	if (!in) {
		return false;
	}

	in >> c0;
	if (!in) {
		return false;
	}

	s = std::string(size, '\0');
	in.read(&s[0], size);
	in >> c1;

	if (!in) {
		return false;
	}
	if (c0 != ':' || c1 != ',') {
		return false;
	}
	return true;
}


bool writeMap(std::string filename, Dmap& map) {
	std::ofstream out;
	out.open(filename, std::ios::out | std::ios::binary);
	if (!out) {
		perror("open");
		return false;
	}

	for (auto &pair : map) {
		auto&k = pair.first;
		auto&v = pair.second;
		if (k.size() == KEYLEN) {
			out << k;
			writeString(out, v);
		}
	}

	out.flush();

	return true;
}

bool readMap(std::string filename, Dmap &map) {
	std::ifstream in;
	in.open(filename);
	if (!in) {
		perror("open");
		return false;
	}

	for (;;) {
		std::string k(KEYLEN, '\0');
		in.read(&k[0], k.size());
		if (in.eof()) {
			break;
		}
		std::string v = "";
		if (!readString(in, v) || in.eof()) {
			goto error;
		}
		map[k] = v;
	}
	return true;

error:
	if (in.bad()) {
		std::cerr << "I/O error when reading D or Dplus\n";
		perror("read");
	} else {
		std::cerr << "syntax error in D or Dplus\n";
	}
	return false;
}

bool writeMapCount(std::string filename, Dcountmap& map) {
	std::ofstream out;
	out.open(filename, std::ios::out | std::ios::binary);
	if (!out) {
		perror("open");
		return false;
	}

	for (auto &pair : map) {
		auto&k = pair.first;
		auto&v = pair.second;
		// Encode token as a netstring
		writeString(out, k);
		// encode count as a decimal number
		out << v;
		// separate by newlines
		out << "\n";
	}

	out.flush();
	return true;
}

bool readMapCount(std::string filename, Dcountmap &map) {
	std::ifstream in;
	in.open(filename);
	if (!in) {
		perror("open");
		return false;
	}

	for (;;) {
		std::string token;
		if (!readString(in, token)) {
			goto error;
		}
		if (in.eof()) {
			break;
		}

		uint64_t v;
		in >> v;
		if (!in) {
			goto error;
		}

		std::cerr << "debug: readMapCount: "<<token << ": " << v << '\n';
		map[token] = v;
	}
	return true;

error:
	if (in.bad()) {
		std::cerr << "I/O error when reading Dcount\n";
		perror("read");
	} else {
		std::cerr << "invalid syntax in Dcount\n";
	}
	return false;
}

bool writeRevlist(std::string filename, Revlist &m) {
	std::ofstream out;
	out.open(filename, std::ios::out | std::ios::binary);
	if (!out) {
		perror("open");
		return false;
	}

	for (auto &v : m) {
		if (v.size() == KEYLEN) {
			out << v;
		}
	}

	out.flush();

	return true;
}

bool readRevlist(std::string filename, Revlist &m) {
	std::ifstream in;
	in.open(filename);
	if (!in) {
		perror("open");
		return false;
	}

	m.clear();
	for (;;) {
		std::string v(KEYLEN, '\0');
		in.read(&v[0], v.size());
		if (in.eof()) {
			break;
		}
		m.insert(v);
	}
	return true;
}

bool writeBytes(std::string filename, const uint8_t* bytes, size_t size) {
	std::ofstream out;
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
	std::ifstream in;
	in.open(filename);
	if (!in) {
		perror("open");
		return false;
	}
	in.read(reinterpret_cast<char*>(bytes), size);
	return in.good() && static_cast<size_t>(in.gcount()) == size;
}


bool writeFileidMap(std::string filename, const std::map<fileid_t, std::string> &m) {
	std::ofstream out;
	out.open(filename, std::ios::binary);
	if (!out) {
		perror("open");
		return false;
	}
	for (auto &pair : m) {
		out << pair.first; // fileid
		out << " ";
		writeString(out, pair.second); // filename
		out << "\n";
	}
	return true;
}

bool readFileidMap(std::string filename, std::map<fileid_t,std::string> &m) {
	std::ifstream in;
	in.open(filename);
	if (!in) {
		perror("open");
		return false;
	}
	for (;;) {
		uint64_t fileid;
		in >> fileid;
		if (in.eof()) {
			break;
		}
		std::string v = "";
		if (!readString(in, v) || in.eof()) {
			goto error;
		}
		m[fileid] = v;
	}
	return true;

error:
	if (in.bad()) {
		std::cerr << "I/O error when reading D or Dplus\n";
		perror("read");
	} else {
		std::cerr << "syntax error in D or Dplus\n";
	}
	return false;
}

bool writeFileid(std::string filename, const fileid_t fileid) {
	std::ofstream out;
	out.open(filename, std::ios::out | std::ios::binary);
	if (!out) {
		perror("open");
		return false;
	}
	out << fileid;
	if (!out) {
		perror("write");
		return false;
	}
	return true;
}

bool readFileid(std::string filename, fileid_t &fileid) {
	std::ifstream in;
	in.open(filename);
	if (!in) {
		perror("open");
		return false;
	}
	in >> fileid;
	if (!in) {
		perror("read");
		return false;
	}
	return true;
}

bool writeInt(std::string filename, int value) {
	std::ofstream out;
	out.open(filename, std::ios::out | std::ios::binary);
	if (!out) {
		perror("open");
		return false;
	}
	out << value;
	if (!out) {
		perror("write");
		return false;
	}
	return true;
}

bool readInt(std::string filename, int &value) {
	std::ifstream in;
	in.open(filename);
	if (!in) {
		perror("open");
		return false;
	}
	in >> value;
	if (!in) {
		perror("read");
		return false;
	}
	return true;
}

bool SaveClientToStorage(DSSE::Core &core, std::string base) {
	if (mkdir(base.c_str(), 0777) < 0) {
		if (errno != EEXIST) {
			perror("mkdir");
			return false;
		}
	}

	if (!writeInt(base+"/version", core.version)) { return false; }
	if (!writeBytes(base+"/key", core.key, KEYLEN)) { std::cerr<<"uhoh\n"; return false; }
	if (!writeBytes(base+"/kplus", core.kplus, KEYLEN)) { return false; }
	if (!writeBytes(base+"/kminus", core.kminus, KEYLEN)) { return false; }
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

	if (!writeInt(base+"/version", core.version)) { return false; }
	if (core.dirtyD) {
		// Only write D after a Setup operation
		if (!writeMap(base+"/D", core.D)) { return false; }
		core.dirtyD = false;
	}
	if (!writeMap(base+"/Dplus", core.Dplus)) { return false; }
	if (!writeRevlist(base+"/Srev", core.Srev)) { return false; }
	return true;
}

bool LoadClientFromStorage(DSSE::Core &core, std::string base) {
	if (!readInt(base+"/version", core.version)) { return false; }
	if (!readBytes(base+"/key", core.key, KEYLEN)) { return false; }
	if (!readBytes(base+"/kplus", core.kplus, KEYLEN)) { return false; }
	if (!readBytes(base+"/kminus", core.kminus, KEYLEN)) { return false; }
	if (!readMapCount(base+"/Dcount", core.Dcount)) { return false; }
	return true;
}

bool LoadServerFromStorage(DSSE::Core &core, std::string base) {
	if (!readInt(base+"/version", core.version)) { return false; }
	if (!readMap(base+"/D", core.D)) { return false; }
	if (!readMap(base+"/Dplus", core.Dplus)) { return false; }
	if (!readRevlist(base+"/Srev", core.Srev)) { return false; }
	return true;
}

bool Client::saveExtraState(std::string base) {
	if (!writeFileidMap(base+"/fileidMap", this->fileidMap)) { return false; }
	if (!writeFileid(base+"/lastFileid", this->lastFileid)) { return false; }
	return true;
}

bool Client::loadExtraState(std::string base) {
	if (!readFileidMap(base+"/fileidMap", this->fileidMap)) { return false; }
	if (!readFileid(base+"/lastFileid", this->lastFileid)) { return false; }
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
