#include <algorithm> // sort
#include <utility> // pair
#include <cstring> // memset
#include <cstdio>
#include <iostream>

#include "DSSE.h"
#include "tomcrypt.h"

namespace DSSE {

const int B = 5; // number of ids in a packed block

// XXX we assume that KEYLEN and DIGESTLEN are equal
const int DIGESTLEN = 256/8;

// size of encrypted file id
const int ENCRYPTED_FILEID_SIZE = sizeof(fileid_t) + 16; // 16 bytes for the IV
const int BLOCK_SIZE = sizeof(fileid_t) * B; // block size in bytes
const int ENCRYPTED_BLOCK_SIZE = sizeof(fileid_t)*B + 16; // 16 bytes for the IV

// placeholder value for invalid fileids or pointers
const uint64_t INVALID = 0xFFFFFFFFFFFFFFFFull;

Core::Core() {
    this->version = Basic;
    this->dirtyD = false;
    this->key = new uint8_t[KEYLEN];
    this->kplus = new uint8_t[KEYLEN];
    this->kminus = new uint8_t[KEYLEN];
    memset(this->key, 0, KEYLEN);
    memset(this->kplus, 0, KEYLEN);
    memset(this->kminus, 0, KEYLEN);
}


// random_bytes gets n random bytes from the OS
// it aborts the program on failure
void random_bytes(uint8_t* buf, ssize_t size) {
    FILE* fp = fopen("/dev/urandom", "rb");
    if (fp == NULL) {
        perror("open /dev/urandom");
        exit(1);
    }
    ssize_t n = size;
    ssize_t nread;
    memset(buf, 0, size);
    while (n > 0) {
        nread = fread(buf, 1, n, fp);
        if (nread < 0) {
            perror("read /dev/urandom");
            exit(1);
        }
        n -= nread;
        buf += nread;
    }
}

// random_key writes a random key into the provided buffer
void random_key(uint8_t key[]) {
    random_bytes(key, KEYLEN);
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

// getu64 unpacks a little-endian 64-bit value from bytes
uint64_t getu64(uint8_t bytes[]) {
    return ((((uint64_t)bytes[0])<<0)
           |(((uint64_t)bytes[1])<<8)
           |(((uint64_t)bytes[2])<<16)
           |(((uint64_t)bytes[3])<<24)
           |(((uint64_t)bytes[4])<<32)
           |(((uint64_t)bytes[5])<<40)
           |(((uint64_t)bytes[6])<<48)
           |(((uint64_t)bytes[7])<<56));
}

// putu64 packs a 64-bit value as a sequence of little-endian bytes
void putu64(uint64_t value, uint8_t bytes[]) {
    bytes[0] = value&0xff;
    bytes[1] = (value>>8)&0xff;
    bytes[2] = (value>>16)&0xff;
    bytes[3] = (value>>24)&0xff;
    bytes[4] = (value>>32)&0xff;
    bytes[5] = (value>>40)&0xff;
    bytes[6] = (value>>48)&0xff;
    bytes[7] = (value>>56)&0xff;
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
    putu64(counter, buf);
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
    uint8_t bytes[8];
    putu64(value, bytes);
    encrypt_bytes(key, bytes, 8, out);
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
    decrypt_bytes(key, ctext, ENCRYPTED_FILEID_SIZE, outbytes);
    out = getu64(outbytes);
}

// bstring constructs a std::string from bytes
std::string bstring(uint8_t bytes[], size_t size) {
    return std::string(reinterpret_cast<char*>(bytes), size);
}

struct token_pair {
    std::string *w; // token
    uint8_t l[DIGESTLEN]; // hashed token
    uint8_t d[ENCRYPTED_BLOCK_SIZE]; // encrypted fileid
    uint8_t r[DIGESTLEN]; // revocation token
};

// Reports whether a comes before b
// TODO: make this constant-time?
bool compare_token_pair(const token_pair& a, const token_pair& b) {
    size_t i;
    for (i = 0; i < sizeof a.l; i++) {
        if (a.l[i] < b.l[i]) {
            return true;
        }
        if (a.l[i] > b.l[i]) {
            return false;
        }
    }
    for (i = 0; i < sizeof a.d; i++) {
        if (a.d[i] < b.d[i]) {
            return true;
        }
        if (a.d[i] > b.d[i]) {
            return false;
        }
    }
    for (i = 0; i < sizeof a.r; i++) {
        if (a.r[i] < b.r[i]) {
            return true;
        }
        if (a.r[i] > b.r[i]) {
            return false;
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
    int version,
    std::vector<std::string> &tokens,
    std::map<std::string, std::vector<fileid_t> > &fileids,
    std::vector<SetupPair> &Loutput,
    std::vector<std::string> &Moutput
) {
    if (version == Basic) {
        this->SetupClientBasic(tokens, fileids, Loutput);
    } else if (version == Packed) {
        this->SetupClientPacked(tokens, fileids, Loutput);
    } else if (version == Pointer) {
        this->SetupClientPointer(tokens, fileids, Loutput, Moutput);
    } else {
        fprintf(stderr, "error: unknown version %d\n", version);
        exit(1);
    }
    this->version = version;
}

void Core::SetupClientBasic(
    std::vector<std::string> &tokens,
    std::map<std::string, std::vector<fileid_t> > &fileids,
    std::vector<SetupPair> &Loutput
) {
    // XXX should setup take an entropy source?

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
            putu64(c, counter_bytes);

            uint8_t fileid_bytes[8];
            fileid_t fid = fids.at(c);
            putu64(fid, fileid_bytes);

            token_pair p = {0};
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
            std::string((char*)p.d, ENCRYPTED_FILEID_SIZE)
        });
    }
}

void Core::SetupClientPacked(
    std::vector<std::string> &tokens,
    std::map<std::string, std::vector<fileid_t> > &fileids,
    std::vector<SetupPair> &Loutput
) {
    // XXX should setup take an entropy source?

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
        for (size_t c = 0; c < ((fids.size()+(B-1))/B); c++) {
            uint8_t counter_bytes[8];
            putu64(c, counter_bytes);

            // We want to make this contain up to 5 different file ids
            uint8_t fileid_bytes[BLOCK_SIZE];
            for (size_t fidc = 0; fidc < B; fidc++) {
                size_t offset = fidc * 8;
                if (c*B+fidc < fids.size()) {
                    fileid_t fid = fids.at(c*B+fidc);
                    putu64(fid, &fileid_bytes[offset]);
                } else {
                    memset(fileid_bytes+offset, 0xff, 8);
                }
            }

            token_pair p = {0};
            mac_counter(K1, counter_bytes, sizeof counter_bytes / 1, p.l);
            encrypt_bytes(K2, fileid_bytes, sizeof fileid_bytes, p.d);

            L.push_back( p );
        }
    }

    std::sort(L.begin(), L.end(), compare_token_pair);

    // hand L off to the server
    for (token_pair &p : L) {
        Loutput.push_back(SetupPair{
            std::string((char*)p.l, sizeof p.l),
            std::string((char*)p.d, ENCRYPTED_BLOCK_SIZE)
        });
    }
}

void Core::SetupClientPointer(
    std::vector<std::string> &tokens,
    std::map<std::string, std::vector<fileid_t> > &fileids,
    std::vector<SetupPair> &Loutput,
    std::vector<std::string> &Moutput
) {
    // initialize client state
    random_key(this->key); // Figure 5 (page 8)
    random_key(this->kplus); // page 17
    random_key(this->kminus); // page 19

    // Figure 5 (page 8)
    auto L = std::vector<token_pair>();
    auto M = std::vector<std::string>();


    for (auto& w : tokens) {
        uint8_t K1[DIGESTLEN], K2[DIGESTLEN];
        mac_key(this->key, '1', w.c_str(), K1);
        mac_key(this->key, '2', w.c_str(), K2);
        auto& fids = fileids.at(w);
        //print_bytes(stdout, "K1", K1, KEYLEN);

        uint8_t buf[ENCRYPTED_BLOCK_SIZE];
        uint8_t pointer_block[BLOCK_SIZE];
        uint8_t fileid_block[BLOCK_SIZE];
        size_t i = 0; // fileid index
        for (uint64_t c = 0; i < fids.size(); c++) {
            for (int j = 0; j < B; j++) {
                // TODO: don't reuse B here for the pointer block length
                for (int k = 0; k < B; k++) {
                    if (i < fids.size()) {
                        fileid_t fid = fids.at(i);
                        putu64(fid, &fileid_block[k * sizeof(fileid_t)]);
                        i++;
                    } else {
                        putu64(INVALID, &fileid_block[k * sizeof(fileid_t)]);
                    }
                }

                // encrypt fileid block and add to M
                // TODO: pick a random index instead of appending
                encrypt_bytes(K2, fileid_block, sizeof fileid_block, buf);
                M.push_back(bstring(buf, sizeof buf));

                // add fileid index to pointer block
                putu64(M.size()-1, &pointer_block[j * sizeof(uint64_t)]);

                // if we're out of fileids,
                // fill remaining pointers with INVALID
                // and break
                if (i >= fids.size()) {
                    for (j = j+1; j < B; j++) {
                        putu64(INVALID, &pointer_block[j * sizeof(uint64_t)]);
                    }
                    break;
                }
            }

            // encrypt pointer block and add to L
            token_pair p = {0};
            uint8_t counter_bytes[8];
            putu64(c, counter_bytes);
            encrypt_bytes(K2, pointer_block, sizeof pointer_block, p.d);
            mac_counter(K1, counter_bytes, sizeof counter_bytes / 1, p.l);
            //print_bytes(stdout, "key", p.l, sizeof p.l);
            L.push_back(p);
        }
    }

    std::sort(L.begin(), L.end(), compare_token_pair);

    // hand L off to the server
    for (token_pair &p : L) {
        Loutput.push_back(SetupPair{
            std::string((char*)p.l, sizeof p.l),
            std::string((char*)p.d, ENCRYPTED_BLOCK_SIZE)
        });
    }
    Moutput = M;
}

void Core::SetupServer(int version, std::vector<SetupPair> &L, std::vector<std::string> &M) {
    this->D.clear();
    this->A.clear();
    this->Dplus.clear(); // page 17
    this->dirtyD = true;

    for (SetupPair& p : L) {
        this->D[p.Token] = p.FileID;
        //print_bytes(stdout, "key", p.Token);
        //print_bytes(stdout, "value", p.FileID);
    }

    this->A = M;
    this->version = version;
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
}

std::vector<uint64_t> Core::SearchServer(key_t K1, key_t K2, key_t K1plus, key_t K2plus, key_t K1minus) {
    if (this->version == Basic) {
        return this->SearchServerBasic(K1, K2, K1plus, K2plus, K1minus);
    } else if (this->version == Packed) {
        return this->SearchServerPacked(K1, K2, K1plus, K2plus, K1minus);
    } else if (this->version == Pointer) {
        return this->SearchServerPointer(K1, K2, K1plus, K2plus, K1minus);
    } else {
        fprintf(stderr, "error: unknown version %d\n", this->version);
        exit(1);
    }
}


std::vector<uint64_t> Core::SearchServerBasic(key_t K1, key_t K2, key_t K1plus, key_t K2plus, key_t K1minus) {
    uint64_t c = 0;
    std::vector<uint64_t> ids;

    std::string revid(KEYLEN, '\0');
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

        // page 20
        // check if id is on the revocation list
        mac_long(K1minus, id, reinterpret_cast<uint8_t*>(&revid[0]));
        if (this->Srev.count(revid) > 0) {
            continue;
        }

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
        uint64_t id = 0;
        decrypt_long(K2plus, reinterpret_cast<const uint8_t*>(d.data()), id);

        // page 20
        // check if id is on the revocation list
        mac_long(K1minus, id, reinterpret_cast<uint8_t*>(&revid[0]));
        if (this->Srev.count(revid) > 0) {
            continue;
        }

        ids.push_back(id);
    }
    return ids;
}


// SearchServer performs the server side of searching the index for a given keyword.
std::vector<uint64_t> Core::SearchServerPacked(key_t K1, key_t K2, key_t K1plus, key_t K2plus, key_t K1minus) {
    uint64_t c = 0;
    std::vector<uint64_t> ids;

    std::string revid(DIGESTLEN, '\0');
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
        uint8_t retid[BLOCK_SIZE];
        decrypt_bytes(K2, reinterpret_cast<const uint8_t*>(d.data()),d.size(), retid);

        //Unpack retid
        for(int i=0; i<B; i++){
            // Gets fid out of retid and converts to int
            uint64_t retval = getu64(&retid[i*8]);
            // check that retval is valid
            if (retval < 0xffffffffffffffff){
                // page 20
                // check if id is on the revocation list
                mac_long(K1minus, retval, reinterpret_cast<uint8_t*>(&revid[0]));
                if (this->Srev.count(revid) > 0) {
                    continue;
                }
                ids.push_back(retval);
            }
        }
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
        uint64_t id = 0;
        decrypt_long(K2plus, reinterpret_cast<const uint8_t*>(d.data()), id);

        // page 20
        // check if id is on the revocation list
        mac_long(K1minus, id, reinterpret_cast<uint8_t*>(&revid[0]));
        if (this->Srev.count(revid) > 0) {
            continue;
        }

        ids.push_back(id);
    }
    return ids;
}

// SearchServer performs the server side of searching the index for a given keyword.
std::vector<uint64_t> Core::SearchServerPointer(key_t K1, key_t K2, key_t K1plus, key_t K2plus, key_t K1minus) {
    uint64_t c = 0;
    std::vector<uint64_t> ids;

    std::string revid(DIGESTLEN, '\0');
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
        uint8_t pointer_block[BLOCK_SIZE];
        decrypt_bytes(K2, reinterpret_cast<const uint8_t*>(d.data()), d.size(), pointer_block);

        for(int i = 0; i < B; i++) {
            uint64_t index = getu64(&pointer_block[i*sizeof(uint64_t)]);

            // check that index is valid
            if (index == INVALID) {
                break;
            }

            if (index >= this->A.size()) {
                // this shouldn't happen
                std::cerr << "error: pointer out of range: "<<index<<" "<<this->A.size()<<"\n";
                continue;
            }

            uint8_t fileid_block[BLOCK_SIZE];
            std::string& encrypted_fileid_block = this->A.at(index);
            decrypt_bytes(K2, reinterpret_cast<const uint8_t*>(encrypted_fileid_block.data()), encrypted_fileid_block.size(), fileid_block);
            for (int j = 0; j < B; j++) {
                uint64_t fileid = getu64(&fileid_block[j*8]);
                if (fileid == INVALID) {
                    continue;
                }
                // check if id is on the revocation list
                mac_long(K1minus, fileid, reinterpret_cast<uint8_t*>(&revid[0]));
                if (this->Srev.count(revid) > 0) {
                    continue;
                }
                ids.push_back(fileid);
            }
        }
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
        uint64_t id = 0;
        decrypt_long(K2plus, reinterpret_cast<const uint8_t*>(d.data()), id);

        // page 20
        // check if id is on the revocation list
        mac_long(K1minus, id, reinterpret_cast<uint8_t*>(&revid[0]));
        if (this->Srev.count(revid) > 0) {
            continue;
        }

        ids.push_back(id);
    }
    return ids;
}


void Core::AddClient(
    // Input
    fileid_t id, std::vector<std::string> words,
    // Output
    std::vector<AddPair> &Loutput,
    std::vector<std::string> &Woutput
) {
    std::vector<token_pair> L;
    for (auto &w : words) {
        key_t K1plus, K2plus;
        key_t K1minus;

        mac_key(this->kplus, '1', w.c_str(), K1plus);
        mac_key(this->kplus, '2', w.c_str(), K2plus);
        mac_key(this->kminus, '1', w.c_str(), K1minus); // p20

        token_pair p;
        uint64_t c = this->Dcount[w];
        mac_long(K1plus, c, p.l);
        encrypt_long(K2plus, id, p.d);

        // if we didn't have to handle deletion, we could do this instead
        //this->Dcount[w]++;
        //L.push_back(p);

        // page 20
        // compute revocation id
        mac_long(K1minus, id, p.r);
        p.w = &w;
        L.push_back(p);
    }

    // Sort L
    std::sort(L.begin(), L.end(), compare_token_pair);

    // copy to output
    Loutput.clear();
    for (token_pair &p : L) {
        AddPair ap;
        ap.Token = std::string(reinterpret_cast<char*>(p.l), sizeof p.l);
        ap.FileID = std::string(reinterpret_cast<char*>(p.d), sizeof p.d);
        ap.RevID = std::string(reinterpret_cast<char*>(p.r), sizeof p.r);
        Loutput.push_back(ap);
        Woutput.push_back(*p.w);
    }
}

void Core::AddServer(std::vector<AddPair> L, std::vector<unsigned char> &r) {
    /*
    for (AddPair &p : L) {
        if (this->Dplus.count(p.Token) > 0) {
            fprintf(stderr, "warning: token already in Dplus\n");
            // XXX abort?
        }
        this->Dplus[p.Token] = p.FileID;
    }
    */
    for (AddPair &p : L) {
        // if token is revoked, remove it from Srev
        // otherwise add it to Dplus
        // return a bitmap of which tokens had been revoked
        if (this->Srev.count(p.RevID) > 0) {
            // page 20
            r.push_back(1);
            this->Srev.erase(p.RevID);
        } else {
            // page 18
            if (this->Dplus.count(p.Token) > 0) {
                fprintf(stderr, "warning: token already in Dplus\n");
            } else {
                this->Dplus[p.Token] = p.FileID;
                r.push_back(0); // p20
            }
        }
    }
}

// the second half of AddClient;
// updates the update count for every non-revoked token
void Core::AddClient2(
    std::vector<unsigned char> r,
    std::vector<std::string> W_in_order_of_Lrev
) {
    // page 20
    for (size_t i = 0; i < r.size(); i++) {
        if (r[i] == 0) {
            std::string &w = W_in_order_of_Lrev.at(i);
            this->Dcount[w]++;
        }
    }
}

void Core::DeleteClient(
    fileid_t id, std::vector<std::string> words,
    std::vector<std::string> &Lrev
) {
    // page 20
    for (auto &w : words) {
        key_t K1minus;
        key_t revidbuf;

        mac_key(this->kminus, '1', w.data(), K1minus);
        mac_long(K1minus, id, revidbuf);

        std::string revid(reinterpret_cast<char*>(&revidbuf[0]), DIGESTLEN);
        Lrev.push_back(revid);
    }
    std::sort(Lrev.begin(), Lrev.end());
}


void Core::DeleteServer(std::vector<std::string> L)
{
    // page 20
    for (auto &revid : L) {
        this->Srev.insert(revid);
    }
}



} // namespace DSSE

/* vim: set expandtab shiftwidth=4: */
