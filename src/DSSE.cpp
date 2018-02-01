#include <algorithm> // sort
#include <utility> // pair
#include <cstring> // memset

#include "DSSE.h"
#include "tomcrypt.h"

namespace DSSE {

// XXX we assume that KEYLEN and DIGESTLEN are equal
const int KEYLEN = 256/8;
const int DIGESTLEN = 256/8;
// size of encrypted file id
const int ENCRYPTLEN = sizeof(fileid_t) + 16; //???

typedef uint8_t key_t[KEYLEN];

Core::Core() {
    this->key = new uint8_t[KEYLEN];
    this->kplus = new uint8_t[KEYLEN];
    this->kminus = new uint8_t[KEYLEN];
    memset(this->key, 0, KEYLEN);
    memset(this->kplus, 0, KEYLEN);
    memset(this->kminus, 0, KEYLEN);
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

void encrypt_bytes(uint8_t key[], uint8_t msg[], size_t msglen, uint8_t out[]){
    symmetric_CTR ctr;
    unsigned char IV[16] = {0};

    // TODO: initialize IV

    memmove(out, IV, 16);

    int num_rounds = 0; // = "use default"
    /* use a 32-bit little endian counter */
    crypt_or_die(ctr_start(find_cipher("aes"),
                IV, key, KEYLEN, num_rounds,
                CTR_COUNTER_LITTLE_ENDIAN,
                &ctr));
    crypt_or_die(ctr_encrypt(msg, out+16, msglen, &ctr));
    crypt_or_die(ctr_done(&ctr));
}

void encrypt_long(uint8_t key[], uint64_t value, uint8_t out[]) {
    // TODO
}

void decrypt_bytes(uint8_t key[], const uint8_t ctext[], size_t ctextlen, uint8_t out[]) {
    symmetric_CTR ctr;
    int num_rounds = 0;
    crypt_or_die(ctr_start(find_cipher("aes"),
        ctext, key, KEYLEN, num_rounds,
        CTR_COUNTER_LITTLE_ENDIAN,
        &ctr));
    crypt_or_die(ctr_decrypt(ctext+16, out, ctextlen-16, &ctr));
    crypt_or_die(ctr_done(&ctr));
}

void decrypt_long(uint8_t key[], const uint8_t ctext[], uint64_t &out) {
    uint8_t outbytes[8];
    decrypt_bytes(key, ctext, ENCRYPTLEN, outbytes);
    out = ((((uint64_t)outbytes[0])<<0)
          |(((uint64_t)outbytes[1])<<8)
          |(((uint64_t)outbytes[2])<<16)
          |(((uint64_t)outbytes[3])<<24)
          |(((uint64_t)outbytes[4])<<32)
          |(((uint64_t)outbytes[5])<<40)
          |(((uint64_t)outbytes[6])<<48)
          |(((uint64_t)outbytes[7])<<56));
}

struct token_pair {
    uint8_t l[DIGESTLEN];
    uint8_t d[ENCRYPTLEN];
};

// Reports whether a comes before b
// TODO: make this constant-time?
bool compare_token_pair(const token_pair& a, const token_pair& b) {
    size_t i;
    for (i = 0; i < sizeof a.l; i++) {
        if (a.l[i] < b.l[i]) {
            return true;
        }
    }
    for (i = 0; i < sizeof a.d; i++) {
        if (a.d[i] < b.d[i]) {
            return true;
        }
    }
    return false;
}

void print_bytes(FILE* fp, const char *title, const uint8_t *data, size_t len) {
    fprintf(fp, "%s: ", title);
    for (size_t i = 0; i < len; i++) {
        fprintf(fp, "%02x", data[i]);
    }
    fprintf(fp, "\n");
}
void print_bytes(FILE* fp, const char *title, std::string s) {
    print_bytes(fp, title, reinterpret_cast<const uint8_t*>(s.data()), s.size());
}

// Setup creates an initial index from a list of tokens and a map of
// file id => token list
void Core::SetupClient(
    std::vector<std::string> &tokens,
    std::map<std::string, std::vector<fileid_t> > &fileids,
    std::vector<SetupPair> &Loutput
) {
    // initialize client state
    random_key(this->key); // Figure 5 (page 8)
    random_key(this->kplus); // page 17
    random_key(this->kminus); // page 19

    // Figure 5 (page 8)
    auto L = std::vector<token_pair>();

    for (auto& w : tokens) {
        uint8_t K1[DIGESTLEN], K2[DIGESTLEN];
        mac_key(this->key, '1', w.c_str(), K1);
        mac_key(this->key, '2', w.c_str(), K2);
        auto& fids = fileids.at(w);
        //print_bytes(stdout, "K1", K1, KEYLEN);
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
            fileid_t fid = fids.at(c);
            fileid_bytes[0] = fid&0xff;
            fileid_bytes[1] = (fid>>8)&0xff;
            fileid_bytes[2] = (fid>>16)&0xff;
            fileid_bytes[3] = (fid>>24)&0xff;
            fileid_bytes[4] = (fid>>32)&0xff;
            fileid_bytes[5] = (fid>>40)&0xff;
            fileid_bytes[6] = (fid>>48)&0xff;
            fileid_bytes[7] = (fid>>56)&0xff;

            token_pair p;
            mac_counter(K1, counter_bytes, sizeof counter_bytes / 1, p.l);
            encrypt_bytes(K2, fileid_bytes, sizeof fileid_bytes / 1, p.d);

            L.push_back( p );
        }
    }

    std::sort(L.begin(), L.end(), compare_token_pair);

    // hand L off to the server
    for (token_pair &p : L) {
        Loutput.push_back(SetupPair{
            std::string((char*)p.l, sizeof p.l),
            std::string((char*)p.d, sizeof p.d)
        });
    }
}

void Core::SetupServer(std::vector<SetupPair> &L) {
    this->D.clear();
    this->Dplus.clear(); // page 17

    for (SetupPair& p : L) {
        this->D[p.Token] = p.FileID;
        //print_bytes(stdout, "key", p.Token);
        //print_bytes(stdout, "value", p.FileID);
    }
}

std::vector<fileid_t> Core::SearchTest(std::string w) {
    // page  8
    key_t K1, K2;
    key_t K1plus, K1minus, K2plus;

    this->SearchClient(w,
        K1, K2, K1plus, K2plus, K1minus);

    return this->SearchServer(K1, K2, K1plus, K2plus, K1minus);
}

void Core::SearchClient(std::string w,
    key_t K1, key_t K2,
    key_t K1plus, key_t K2plus,
    key_t K1minus
) {

    // page  8
    mac_key(this->key, '1', w.c_str(), K1);
    mac_key(this->key, '2', w.c_str(), K2);

    /* adding and deleting */
    // page 18
    mac_key(this->kplus, '1', w.c_str(), K1plus);
    mac_key(this->kplus, '2', w.c_str(), K2plus);

    // page 20
    mac_key(this->kminus, '1', w.c_str(), K1minus);

    // return some sort of message for the server?
    //return self.search_server(K1, K2, K1plus, K2plus, K1minus)
}

// SearchServer performs the server side of searching the index for a given keyword.
std::vector<uint64_t> Core::SearchServer(uint8_t K1[], uint8_t K2[], uint8_t K1plus[], uint8_t K2plus[], uint8_t K1minus[]) {
    uint64_t c = 0;
    std::vector<uint64_t> ids;

    //print_bytes(stdout, "K1", K1, KEYLEN);
    for (c = 0;; c++) {
        uint8_t l[DIGESTLEN];
        std::string d;
        mac_long(K1, c, l);
        //print_bytes(stdout, "key", l, sizeof l);
        try {
            d = this->D.at(std::string(reinterpret_cast<char*>(l), sizeof l / 1));
        } catch (std::out_of_range& e) {
            break;
        }
        uint64_t id;
        decrypt_long(K2, reinterpret_cast<const uint8_t*>(d.data()), id);
        /*
        revid = self._mac(K1minus, id) # p20
        if revid not in self.Srev: # p20
            ids.append(id)
        */
        ids.push_back(id);
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
        decrypt_long(K2plus, reinterpret_cast<const uint8_t*>(d.data()), id);

        /*
        revid = self._mac(K1minus, id) # p20
        if revid not in self.Srev: # p20
            ids.append(id)
        */
        ids.push_back(id);
    }
    return ids;
}


void Core::AddClient(
    // Input
    std::string id, std::vector<std::string> words,
    // Output
    std::vector<AddPair> &Loutput
) {

    std::vector<token_pair> L;
    //W_in_order_of_Lrev = [] // p20

    for (auto &w : words) {
        key_t K1plus, K2plus;
        key_t K1minus;

        mac_key(this->kplus, '1', w.c_str(), K1plus);
        mac_key(this->kplus, '2', w.c_str(), K2plus);
        mac_key(this->kminus, '1', w.c_str(), K1minus); // p20

        token_pair p;
        uint64_t c = this->Dcount[w];
        mac_long(K1plus, c, p.l);
        encrypt_long(K2plus, c, p.d);
        this->Dcount[w]++;

        L.push_back(p); // XXX

        /*
        # page 20
        revid = self._mac(K1minus, id)
        L.append((l, d, revid))
        W_in_order_of_Lrev.append(w)
        */
    }

    // copy to output
    Loutput.clear();
    for (token_pair &p : L) {
        AddPair ap;
        ap.Token = std::string(reinterpret_cast<char*>(p.l), sizeof p.l);
        ap.FileID = std::string(reinterpret_cast<char*>(p.d), sizeof p.d);
        Loutput.push_back(ap);
    }
}

void Core::AddServer(std::vector<AddPair> L) {
    for (AddPair &p : L) {
        if (this->Dplus.count(p.Token) > 0) {
            fprintf(stderr, "warning: token already in Dplus\n");
            // XXX abort?
        }
        this->Dplus[p.Token] = p.FileID;
    }
}

// the second half of AddClient;
// updates the update count for every non-revoked token
void Core::AddClient2(
    std::vector<std::string> r,
    std::vector<std::string> W_in_order_of_Lrev
) {
    /*
    # page 20
    for ri, w in zip(r, W_in_order_of_Lrev):
        if not ri:
            c = self.Dcount.get(w, 0)
            c += 1
            self.Dcount[w] = c
    */
}


} // namespace DSSE

/* vim: set expandtab shiftwidth=4: */
