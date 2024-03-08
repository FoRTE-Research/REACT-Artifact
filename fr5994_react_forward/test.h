#ifndef AES_TEST_H_
#define AES_TEST_H_

/** need to choose which AES implementation to run **/
//#define gladman_aes
//#define slow_tiny_aes
#define tiny_aes
//#define mbedtls_aes

/** need to define key size **/
#define AES_128 1
//#define AES_192 1
//#define AES_256 1

/** can define block chaining **/
#define AES_CBC
#define MSG_LNGTH 1024

/** AES constants **/
#define AES_BLOCK_SIZE_BITS 128
#define AES_BLOCK_SIZE_BYTES (AES_BLOCK_SIZE_BITS/8)



#endif /* AES_TEST_H_ */
