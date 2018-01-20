#include <algorithm> // sort
#include <utility> // pair
#include <cstring> // memset

#include "DSSE.h"
#include "tomcrypt.h"

// XXX we assume that KEYLEN and DIGESTLEN are equal
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
 * If err is not CRYPT_OK, prints the error and calls exit(1).
 */
void crypt_or_die(int err) {
    if (err != CRYPT_OK) {
        fprintf(stderr, "error: %s\n", error_to_string(err));
        exit(1);
    }
}

/**
 * mac_key uses HMAC to derive a per-token key from a string token
 */
void mac_key(uint8_t key[], char keynum, const char* token, uint8_t out[]) {
    hmac_state hmac;
    int hash = find_hash("sha256");
    crypt_or_die(hmac_init(&hmac, hash, key, KEYLEN));
    unsigned char keynumbuf[1] = {(unsigned char)keynum};
    crypt_or_die(hmac_process(&hmac, keynumbuf, 1));
    crypt_or_die(hmac_process(&hmac, (const unsigned char*)token, strlen(token)));
    unsigned long outlen = DIGESTLEN;
    crypt_or_die(hmac_done(&hmac, out, &outlen));
}

/**
 * mac_counter uses a per-token key to derive a hashed id value
 */
void mac_counter(uint8_t key[], uint8_t message[], size_t msglen, uint8_t out[]) {
    hmac_state hmac;
    int hash = find_hash("sha256");
    crypt_or_die(hmac_init(&hmac, hash, key, KEYLEN));
    crypt_or_die(hmac_process(&hmac, message, msglen));
    unsigned long outlen = DIGESTLEN;
    crypt_or_die(hmac_done(&hmac, out, &outlen));
    // TODO check outlen
}

/**
 * mac_long uses a per-token key to derive a hashed id value
 */
void mac_long(uint8_t key[], uint64_t counter, uint8_t out[]) {
    hmac_state hmac;
    int hash = find_hash("sha256");
    crypt_or_die(hmac_init(&hmac, hash, key, KEYLEN));
    uint8_t buf[8];
    buf[0] = counter&0xff;
    buf[1] = (counter>>8)&0xff;
    buf[2] = (counter>>16)&0xff;
    buf[3] = (counter>>24)&0xff;
    buf[4] = (counter>>32)&0xff;
    buf[5] = (counter>>40)&0xff;
    buf[6] = (counter>>48)&0xff;
    buf[7] = (counter>>56)&0xff;
    crypt_or_die(hmac_process(&hmac, buf, sizeof buf / 1));
    unsigned long outlen = DIGESTLEN;
    crypt_or_die(hmac_done(&hmac, out, &outlen));
    // TODO check outlen
}

void decrypt_long(uint8_t key[], std::string hi, uint64_t &out) {
    /*TODO*/
}

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
    for (token_pair& p: L) {
        std::pair<std::string,std::string> v(
            std::string((char*)p.l, sizeof p.l),
            std::string((char*)p.d, sizeof p.d)
        );
        this->D.insert(v);
    }
}


void DSSE::SearchClient(std::string w) {
    typedef uint8_t key_t[KEYLEN];

    // page  8
    key_t K1, K2;
    mac_key(this->key, '1', w.c_str(), K1);
    mac_key(this->key, '2', w.c_str(), K2);

    /* adding and deleting */
    /*
    // page 18
    key_t K1plus, K2plus;
    mac_key(this->kplus, '1', w.c_str(), K1plus);
    mac_key(this->kplus, '2', w.c_str(), K2plus);

    // page 20
    key_t K1minus;
    mac_key(this->kminus, '1', K1minus);
    */

    // return some sort of message for the server?
    //return self.search_server(K1, K2, K1plus, K2plus, K1minus)
}

// SearchServer performs the server side of searching the index for a given keyword.
std::vector<uint64_t> DSSE::SearchServer(uint8_t K1[], uint8_t K2[], uint8_t K1plus[], uint8_t K2plus[], uint8_t K1minus[]) {
    uint64_t c = 0;
    std::vector<uint64_t> ids;

    for (c = 0;; c++) {
        uint8_t l[DIGESTLEN];
        std::string d;
        mac_long(K1, c, l);
        try {
            d = this->D.at(std::string(reinterpret_cast<char*>(l), sizeof l / 1));
        } catch (std::out_of_range& e) {
            break;
        }
        uint64_t id;
        decrypt_long(K2, d, id);
        /*
        revid = self._mac(K1minus, id) # p20
        if revid not in self.Srev: # p20
            ids.append(id)
        */
    }

    // page 18
    for (c = 0;; c++) {
        uint8_t l[DIGESTLEN];
        std::string d;
        mac_long(K1plus, c, l);
        try {
            d = this->Dplus.at(std::string(reinterpret_cast<char*>(l), sizeof l / 1));
        } catch (std::out_of_range& e) {
            break;
        }
        uint64_t id;
        decrypt_long(K2plus, d, id);

        /*
        revid = self._mac(K1minus, id) # p20
        if revid not in self.Srev: # p20
            ids.append(id)
        */
    }
    return ids;
}


/* vim: set expandtab shiftwidth=4: */
