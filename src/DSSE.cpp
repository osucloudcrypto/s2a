#include <algorithm> // sort
#include <cstring> // memset

#include "DSSE.h"

const int KEYLEN = 256/8;
const int DIGESTLEN = 256/8;

void DSSE::Init() {
    this->key = new uint8_t[KEYLEN];
}


// random_key writes a random key into theprovided buffer
void random_key(uint8_t key[]) {
    // TODO
    memset(key, 0, KEYLEN);
}

/**
 * mac_key uses HMAC to derive a per-token key from a string token
 */
void mac_key(uint8_t key[], char keynum, const char* token, uint8_t out[]) {}

/**
 * mac_counter uses a per-token key to derive a hashed id value
 */
void mac_counter(uint8_t key[], uint8_t message[], size_t msglen, uint8_t out[]) {}


struct token_pair {
    uint8_t l[8];
    uint8_t d[8];
};

// Reports whether a comes before b
// TODO: make this constant-time?
bool compare_token_pair(const token_pair& a, const token_pair& b) {
    int i;
    for (i = 0; i < 8; i++) {
        if (a.l[i] < b.l[i]) {
            return true;
        }
    }
    for (i = 0; i < 8; i++) {
        if (a.d[i] < b.d[i]) {
            return true;
        }
    }
    return false;
}

// Setup creates an initial index from a list of tokens and a map of
// file id => token list
void DSSE::Setup(std::vector<std::string> &tokens, std::map<std::string, std::vector<std::string> > &fileids) {

    // Figure 5 (page 8)
    random_key(this->key);

    auto L = std::vector<token_pair>();

    for (auto& w : tokens) {
        uint8_t K1[DIGESTLEN], K2[DIGESTLEN];
        mac_key(this->key, '1', w.c_str(), K1);
        mac_key(this->key, '2', w.c_str(), K2);
        auto& fids = fileids.at(w);
        for (size_t c = 0; c < fids.size(); c++) {
            uint8_t counter_bytes[8];
            counter_bytes[0] = c&0xff;
            counter_bytes[1] = (c>>8)&0xff;
            counter_bytes[2] = (c>>16)&0xff;
            counter_bytes[3] = (c>>24)&0xff;
            counter_bytes[4] = (c>>32)&0xff;
            counter_bytes[5] = (c>>40)&0xff;
            counter_bytes[6] = (c>>48)&0xff;
            counter_bytes[7] = (c>>56)&0xff;

            uint8_t fileid_bytes[8];
            fileid_bytes[0] = c&0xff;
            fileid_bytes[1] = (c>>8)&0xff;
            fileid_bytes[2] = (c>>16)&0xff;
            fileid_bytes[3] = (c>>24)&0xff;
            fileid_bytes[4] = (c>>32)&0xff;
            fileid_bytes[5] = (c>>40)&0xff;
            fileid_bytes[6] = (c>>48)&0xff;
            fileid_bytes[7] = (c>>56)&0xff;

            token_pair p;
            mac_counter(K1, counter_bytes, sizeof counter_bytes / 1, p.l);
            mac_counter(K2, fileid_bytes, sizeof fileid_bytes / 1, p.d);

            L.push_back( p );
        }
    }

    std::sort(L.begin(), L.end(), compare_token_pair);

    // send L to server
    // and do this on the server side:
    //  self.D = dict(L) # server state

}
