#include "DSSE.h"
#include "dsse.pb.h"

std::vector<DSSE::fileid_t>
DSSE::Client::Search(std::string w) {
	DSSE::msg::Search msg;
	msg.set_k1("hi");
	return std::vector<DSSE::fileid_t>{3};
}
