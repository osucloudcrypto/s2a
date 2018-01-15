/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 */

/**
   @file ocb3_test.c
   OCB implementation, self-test by Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_OCB3_MODE

/**
   Test the OCB protocol
   @return CRYPT_OK if successful
*/
int ocb3_test(void)
{
#ifndef LTC_TEST
   return CRYPT_NOP;
#else
   /* test vectors from: http://tools.ietf.org/html/draft-krovetz-ocb-03 */
   unsigned char key[16]   = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F };
   unsigned char nonce[12] = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B };
   const struct {
         int ptlen;
         int aadlen;
         unsigned char pt[64], aad[64], ct[64], tag[16];
   } tests[] = {

   { /* index:0 */
     0, /* PLAINTEXT length */
     0, /* AAD length */
     { 0 }, /* PLAINTEXT */
     { 0 }, /* AAD */
     { 0 }, /* CIPHERTEXT */
     { 0x19,0x7b,0x9c,0x3c,0x44,0x1d,0x3c,0x83,0xea,0xfb,0x2b,0xef,0x63,0x3b,0x91,0x82 }, /* TAG */
   },
   { /* index:1 */
     8, /* PLAINTEXT length */
     8, /* AAD length */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07 }, /* PLAINTEXT */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07 }, /* AAD */
     { 0x92,0xb6,0x57,0x13,0x0a,0x74,0xb8,0x5a }, /* CIPHERTEXT */
     { 0x16,0xdc,0x76,0xa4,0x6d,0x47,0xe1,0xea,0xd5,0x37,0x20,0x9e,0x8a,0x96,0xd1,0x4e }, /* TAG */
   },
   { /* index:2 */
     0, /* PLAINTEXT length */
     8, /* AAD length */
     { 0 }, /* PLAINTEXT */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07 }, /* AAD */
     { 0 }, /* CIPHERTEXT */
     { 0x98,0xb9,0x15,0x52,0xc8,0xc0,0x09,0x18,0x50,0x44,0xe3,0x0a,0x6e,0xb2,0xfe,0x21 }, /* TAG */
   },
   { /* index:3 */
     8, /* PLAINTEXT length */
     0, /* AAD length */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07 }, /* PLAINTEXT */
     { 0 }, /* AAD */
     { 0x92,0xb6,0x57,0x13,0x0a,0x74,0xb8,0x5a }, /* CIPHERTEXT */
     { 0x97,0x1e,0xff,0xca,0xe1,0x9a,0xd4,0x71,0x6f,0x88,0xe8,0x7b,0x87,0x1f,0xbe,0xed }, /* TAG */
   },
   { /* index:4 */
     16, /* PLAINTEXT length */
     16, /* AAD length */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f }, /* PLAINTEXT */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f }, /* AAD */
     { 0xbe,0xa5,0xe8,0x79,0x8d,0xbe,0x71,0x10,0x03,0x1c,0x14,0x4d,0xa0,0xb2,0x61,0x22 }, /* CIPHERTEXT */
     { 0x77,0x6c,0x99,0x24,0xd6,0x72,0x3a,0x1f,0xc4,0x52,0x45,0x32,0xac,0x3e,0x5b,0xeb }, /* TAG */
   },
   { /* index:5 */
     0, /* PLAINTEXT length */
     16, /* AAD length */
     { 0 }, /* PLAINTEXT */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f }, /* AAD */
     { 0 }, /* CIPHERTEXT */
     { 0x7d,0xdb,0x8e,0x6c,0xea,0x68,0x14,0x86,0x62,0x12,0x50,0x96,0x19,0xb1,0x9c,0xc6 }, /* TAG */
   },
   { /* index:6 */
     16, /* PLAINTEXT length */
     0, /* AAD length */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f }, /* PLAINTEXT */
     { 0 }, /* AAD */
     { 0xbe,0xa5,0xe8,0x79,0x8d,0xbe,0x71,0x10,0x03,0x1c,0x14,0x4d,0xa0,0xb2,0x61,0x22 }, /* CIPHERTEXT */
     { 0x13,0xcc,0x8b,0x74,0x78,0x07,0x12,0x1a,0x4c,0xbb,0x3e,0x4b,0xd6,0xb4,0x56,0xaf }, /* TAG */
   },
   { /* index:7 */
     24, /* PLAINTEXT length */
     24, /* AAD length */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17 }, /* PLAINTEXT */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17 }, /* AAD */
     { 0xbe,0xa5,0xe8,0x79,0x8d,0xbe,0x71,0x10,0x03,0x1c,0x14,0x4d,0xa0,0xb2,0x61,0x22,0xfc,0xfc,0xee,0x7a,0x2a,0x8d,0x4d,0x48 }, /* CIPHERTEXT */
     { 0x5f,0xa9,0x4f,0xc3,0xf3,0x88,0x20,0xf1,0xdc,0x3f,0x3d,0x1f,0xd4,0xe5,0x5e,0x1c }, /* TAG */
   },
   { /* index:8 */
     0, /* PLAINTEXT length */
     24, /* AAD length */
     { 0 }, /* PLAINTEXT */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17 }, /* AAD */
     { 0 }, /* CIPHERTEXT */
     { 0x28,0x20,0x26,0xda,0x30,0x68,0xbc,0x9f,0xa1,0x18,0x68,0x1d,0x55,0x9f,0x10,0xf6 }, /* TAG */
   },
   { /* index:9 */
     24, /* PLAINTEXT length */
     0, /* AAD length */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17 }, /* PLAINTEXT */
     { 0 }, /* AAD */
     { 0xbe,0xa5,0xe8,0x79,0x8d,0xbe,0x71,0x10,0x03,0x1c,0x14,0x4d,0xa0,0xb2,0x61,0x22,0xfc,0xfc,0xee,0x7a,0x2a,0x8d,0x4d,0x48 }, /* CIPHERTEXT */
     { 0x6e,0xf2,0xf5,0x25,0x87,0xfd,0xa0,0xed,0x97,0xdc,0x7e,0xed,0xe2,0x41,0xdf,0x68 }, /* TAG */
   },
   { /* index:10 */
     32, /* PLAINTEXT length */
     32, /* AAD length */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f }, /* PLAINTEXT */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f }, /* AAD */
     { 0xbe,0xa5,0xe8,0x79,0x8d,0xbe,0x71,0x10,0x03,0x1c,0x14,0x4d,0xa0,0xb2,0x61,0x22,0xce,0xaa,0xb9,0xb0,0x5d,0xf7,0x71,0xa6,0x57,0x14,0x9d,0x53,0x77,0x34,0x63,0xcb }, /* CIPHERTEXT */
     { 0xb2,0xa0,0x40,0xdd,0x3b,0xd5,0x16,0x43,0x72,0xd7,0x6d,0x7b,0xb6,0x82,0x42,0x40 }, /* TAG */
   },
   { /* index:11 */
     0, /* PLAINTEXT length */
     32, /* AAD length */
     { 0 }, /* PLAINTEXT */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f }, /* AAD */
     { 0 }, /* CIPHERTEXT */
     { 0xe1,0xe0,0x72,0x63,0x3b,0xad,0xe5,0x1a,0x60,0xe8,0x59,0x51,0xd9,0xc4,0x2a,0x1b }, /* TAG */
   },
   { /* index:12 */
     32, /* PLAINTEXT length */
     0, /* AAD length */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f }, /* PLAINTEXT */
     { 0 }, /* AAD */
     { 0xbe,0xa5,0xe8,0x79,0x8d,0xbe,0x71,0x10,0x03,0x1c,0x14,0x4d,0xa0,0xb2,0x61,0x22,0xce,0xaa,0xb9,0xb0,0x5d,0xf7,0x71,0xa6,0x57,0x14,0x9d,0x53,0x77,0x34,0x63,0xcb }, /* CIPHERTEXT */
     { 0x4a,0x3b,0xae,0x82,0x44,0x65,0xcf,0xda,0xf8,0xc4,0x1f,0xc5,0x0c,0x7d,0xf9,0xd9 }, /* TAG */
   },
   { /* index:13 */
     40, /* PLAINTEXT length */
     40, /* AAD length */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27 }, /* PLAINTEXT */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27 }, /* AAD */
     { 0xbe,0xa5,0xe8,0x79,0x8d,0xbe,0x71,0x10,0x03,0x1c,0x14,0x4d,0xa0,0xb2,0x61,0x22,0xce,0xaa,0xb9,0xb0,0x5d,0xf7,0x71,0xa6,0x57,0x14,0x9d,0x53,0x77,0x34,0x63,0xcb,0x68,0xc6,0x57,0x78,0xb0,0x58,0xa6,0x35 }, /* CIPHERTEXT */
     { 0x65,0x9c,0x62,0x32,0x11,0xde,0xea,0x0d,0xe3,0x0d,0x2c,0x38,0x18,0x79,0xf4,0xc8 }, /* TAG */
   },
   { /* index:14 */
     0, /* PLAINTEXT length */
     40, /* AAD length */
     { 0 }, /* PLAINTEXT */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27 }, /* AAD */
     { 0 }, /* CIPHERTEXT */
     { 0x7a,0xeb,0x7a,0x69,0xa1,0x68,0x7d,0xd0,0x82,0xca,0x27,0xb0,0xd9,0xa3,0x70,0x96 }, /* TAG */
   },
   { /* index:15 */
     40, /* PLAINTEXT length */
     0, /* AAD length */
     { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27 }, /* PLAINTEXT */
     { 0 }, /* AAD */
     { 0xbe,0xa5,0xe8,0x79,0x8d,0xbe,0x71,0x10,0x03,0x1c,0x14,0x4d,0xa0,0xb2,0x61,0x22,0xce,0xaa,0xb9,0xb0,0x5d,0xf7,0x71,0xa6,0x57,0x14,0x9d,0x53,0x77,0x34,0x63,0xcb,0x68,0xc6,0x57,0x78,0xb0,0x58,0xa6,0x35 }, /* CIPHERTEXT */
     { 0x06,0x0c,0x84,0x67,0xf4,0xab,0xab,0x5e,0x8b,0x3c,0x20,0x67,0xa2,0xe1,0x15,0xdc }, /* TAG */
   },

};
   /* As of RFC 7253 - 'Appendix A.  Sample Results'
    *    The next tuple shows a result with a tag length of 96 bits and a
   different key.

     K: 0F0E0D0C0B0A09080706050403020100

     N: BBAA9988776655443322110D
     A: 000102030405060708090A0B0C0D0E0F1011121314151617
        18191A1B1C1D1E1F2021222324252627
     P: 000102030405060708090A0B0C0D0E0F1011121314151617
        18191A1B1C1D1E1F2021222324252627
     C: 1792A4E31E0755FB03E31B22116E6C2DDF9EFD6E33D536F1
        A0124B0A55BAE884ED93481529C76B6AD0C515F4D1CDD4FD
        AC4F02AA

        The C has been split up in C and T (tag)
    */
   const unsigned char K[] = { 0x0F,0x0E,0x0D,0x0C,0x0B,0x0A,0x09,0x08,
                               0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00 };
   const unsigned char N[] = { 0xBB,0xAA,0x99,0x88,0x77,0x66,0x55,0x44,
                               0x33,0x22,0x11,0x0D };
   const unsigned char A[] = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                               0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
                               0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
                               0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
                               0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27 };
   const unsigned char P[] = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                               0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
                               0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
                               0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
                               0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27 };
   const unsigned char C[] = { 0x17,0x92,0xA4,0xE3,0x1E,0x07,0x55,0xFB,
                               0x03,0xE3,0x1B,0x22,0x11,0x6E,0x6C,0x2D,
                               0xDF,0x9E,0xFD,0x6E,0x33,0xD5,0x36,0xF1,
                               0xA0,0x12,0x4B,0x0A,0x55,0xBA,0xE8,0x84,
                               0xED,0x93,0x48,0x15,0x29,0xC7,0x6B,0x6A };
   const unsigned char T[] = { 0xD0,0xC5,0x15,0xF4,0xD1,0xCD,0xD4,0xFD,
                               0xAC,0x4F,0x02,0xAA };

   int err, x, idx, res;
   unsigned long len;
   unsigned char outct[MAXBLOCKSIZE]  = { 0 };
   unsigned char outtag[MAXBLOCKSIZE] = { 0 };
   ocb3_state ocb;

    /* AES can be under rijndael or aes... try to find it */
    if ((idx = find_cipher("aes")) == -1) {
       if ((idx = find_cipher("rijndael")) == -1) {
          return CRYPT_NOP;
       }
    }

    for (x = 0; x < (int)(sizeof(tests)/sizeof(tests[0])); x++) {
        len = 16; /* must be the same as the required taglen */
        if ((err = ocb3_encrypt_authenticate_memory(idx,
                                                   key, sizeof(key),
                                                   nonce, sizeof(nonce),
                                                   tests[x].aadlen != 0 ? tests[x].aad : NULL, tests[x].aadlen,
                                                   tests[x].ptlen != 0 ? tests[x].pt : NULL, tests[x].ptlen,
                                                   tests[x].ptlen != 0 ? outct : NULL, outtag, &len)) != CRYPT_OK) {
           return err;
        }

        if (compare_testvector(outtag, len, tests[x].tag, sizeof(tests[x].tag), "OCB3 Tag", x) ||
              compare_testvector(outct, tests[x].ptlen, tests[x].ct, tests[x].ptlen, "OCB3 CT", x)) {
           return CRYPT_FAIL_TESTVECTOR;
        }

        if ((err = ocb3_decrypt_verify_memory(idx,
                                             key, sizeof(key),
                                             nonce, sizeof(nonce),
                                             tests[x].aadlen != 0 ? tests[x].aad : NULL, tests[x].aadlen,
                                             tests[x].ptlen != 0 ? outct : NULL, tests[x].ptlen,
                                             tests[x].ptlen != 0 ? outct : NULL, tests[x].tag, len, &res)) != CRYPT_OK) {
           return err;
        }
        if ((res != 1) || compare_testvector(outct, tests[x].ptlen, tests[x].pt, tests[x].ptlen, "OCB3", x)) {
#ifdef LTC_TEST_DBG
           printf("\n\nOCB3: Failure-decrypt - res = %d\n", res);
#endif
           return CRYPT_FAIL_TESTVECTOR;
        }
    }

    /* RFC 7253 - test vector with a tag length of 96 bits - part 1 */
    x = 99;
    len = 12;
    if ((err = ocb3_encrypt_authenticate_memory(idx,
                                                K, sizeof(K),
                                                N, sizeof(N),
                                                A, sizeof(A),
                                                P, sizeof(P),
                                                outct, outtag, &len)) != CRYPT_OK) {
       return err;
    }

    if (compare_testvector(outtag, len, T, sizeof(T), "OCB3 Tag", x) ||
          compare_testvector(outct, sizeof(P), C, sizeof(C), "OCB3 CT", x)) {
       return CRYPT_FAIL_TESTVECTOR;
    }

    if ((err = ocb3_decrypt_verify_memory(idx,
                                          K, sizeof(K),
                                          N, sizeof(N),
                                          A, sizeof(A),
                                          C, sizeof(C),
                                          outct, T, sizeof(T), &res)) != CRYPT_OK) {
       return err;
    }
    if ((res != 1) || compare_testvector(outct, sizeof(C), P, sizeof(P), "OCB3", x)) {
#ifdef LTC_TEST_DBG
       printf("\n\nOCB3: Failure-decrypt - res = %d\n", res);
#endif
       return CRYPT_FAIL_TESTVECTOR;
    }

    /* RFC 7253 - test vector with a tag length of 96 bits - part 2 */
    x = 100;
    if ((err = ocb3_init(&ocb, idx, K, sizeof(K), N, sizeof(N), 12)) != CRYPT_OK)  return err;
    if ((err = ocb3_add_aad(&ocb, A, sizeof(A))) != CRYPT_OK)                      return err;
    if ((err = ocb3_encrypt(&ocb, P, 32, outct)) != CRYPT_OK)                      return err;
    if ((err = ocb3_encrypt_last(&ocb, P+32, sizeof(P)-32, outct+32)) != CRYPT_OK) return err;
    len = sizeof(outtag); /* intentionally more than 12 */
    if ((err = ocb3_done(&ocb, outtag, &len)) != CRYPT_OK)                         return err;
    if (compare_testvector(outct, sizeof(P), C, sizeof(C), "OCB3 CT", x))          return CRYPT_FAIL_TESTVECTOR;
    if (compare_testvector(outtag, len, T, sizeof(T), "OCB3 Tag.enc", x))          return CRYPT_FAIL_TESTVECTOR;
    if ((err = ocb3_init(&ocb, idx, K, sizeof(K), N, sizeof(N), 12)) != CRYPT_OK)  return err;
    if ((err = ocb3_add_aad(&ocb, A, sizeof(A))) != CRYPT_OK)                      return err;
    if ((err = ocb3_decrypt(&ocb, C, 32, outct)) != CRYPT_OK)                      return err;
    if ((err = ocb3_decrypt_last(&ocb, C+32, sizeof(C)-32, outct+32)) != CRYPT_OK) return err;
    len = sizeof(outtag); /* intentionally more than 12 */
    if ((err = ocb3_done(&ocb, outtag, &len)) != CRYPT_OK)                         return err;
    if (compare_testvector(outct, sizeof(C), P, sizeof(P), "OCB3 PT", x))          return CRYPT_FAIL_TESTVECTOR;
    if (compare_testvector(outtag, len, T, sizeof(T), "OCB3 Tag.dec", x))          return CRYPT_FAIL_TESTVECTOR;

    return CRYPT_OK;
#endif /* LTC_TEST */
}

#endif /* LTC_OCB3_MODE */

/* ref:         HEAD -> master, tag: v1.18.0 */
/* git commit:  0676c9aec7299f5c398d96cbbb64f7e38f67d73f */
/* commit time: 2017-10-10 15:51:36 +0200 */
