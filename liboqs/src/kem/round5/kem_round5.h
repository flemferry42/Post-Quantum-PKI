#ifndef __OQS_KEM_ROUND5_H
#define __OQS_KEM_ROUND5_H

#include <oqs/oqs.h>

#ifdef OQS_ENABLE_KEM_round5

#define OQS_KEM_sike_length_public_key 378
#define OQS_KEM_sike_length_secret_key 434
#define OQS_KEM_sike_length_ciphertext 402
#define OQS_KEM_sike_length_shared_secret 16

OQS_KEM *OQS_KEM_round5_p503_new();

OQS_STATUS OQS_KEM_round5_keypair(uint8_t *public_key, uint8_t *secret_key);
OQS_STATUS OQS_KEM_round5_encaps(uint8_t *ciphertext, uint8_t *shared_secret, const uint8_t *public_key);
OQS_STATUS OQS_KEM_round5_decaps(uint8_t *shared_secret, const unsigned char *ciphertext, const uint8_t *secret_key);

#endif

#ifdef OQS_ENABLE_KEM_round5

#define OQS_KEM_sike_p751_length_public_key 564
#define OQS_KEM_sike_p751_length_secret_key 644
#define OQS_KEM_sike_p751_length_ciphertext 596
#define OQS_KEM_sike_p751_length_shared_secret 24

OQS_KEM *OQS_KEM_sike_p751_new();

OQS_STATUS OQS_KEM_sike_p751_keypair(uint8_t *public_key, uint8_t *secret_key);
OQS_STATUS OQS_KEM_sike_p751_encaps(uint8_t *ciphertext, uint8_t *shared_secret, const uint8_t *public_key);
OQS_STATUS OQS_KEM_sike_p751_decaps(uint8_t *shared_secret, const unsigned char *ciphertext, const uint8_t *secret_key);

#endif

#endif
