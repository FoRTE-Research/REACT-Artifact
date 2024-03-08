#ifndef _AES_H_
#define _AES_H_

#include <stdint.h>
#include <stddef.h>

/*****************************************************************************/
/* Defines:                                                                  */
/*****************************************************************************/
// The number of columns comprising a state in AES. This is a constant in AES. Value=4
#define Nb 4

#if defined(AES_256) && (AES_256 == 1)
    #define Nk 8
    #define Nr 14
#elif defined(AES_192) && (AES_192 == 1)
    #define Nk 6
    #define Nr 12
#else
    #define Nk 4        // The number of 32 bit words in a key.
    #define Nr 10       // The number of rounds in AES Cipher.
#endif

#define AES_BLOCKLEN 16 // Block length in bytes - AES is 128b block only

#if defined(AES_256) && (AES_256 == 1)
    #define AES_KEYLEN 32
    #define AES_keyExpSize 240
#elif defined(AES_192) && (AES_192 == 1)
    #define AES_KEYLEN 24
    #define AES_keyExpSize 208
#else
    #define AES_KEYLEN 16   // Key length in bytes
    #define AES_keyExpSize 176
#endif

struct AES_ctx
{
  uint8_t RoundKey[AES_keyExpSize];
};

void AES_init_ctx(struct AES_ctx* ctx, const uint8_t* key);
void AES_encrypt(struct AES_ctx* ctx, uint8_t key[], uint8_t in[], uint8_t out[]);
uint8_t * AES_decrypt(struct AES_ctx* ctx, uint8_t key[], uint8_t out[], uint8_t in[]);

#endif // _AES_H_
