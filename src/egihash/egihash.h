// Copyright (c) 2017 Ryan Lucchese
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>
extern "C" 
{
   #include "keccak-tiny.h"
}
#include "secure_memzero.h"

#ifdef __cplusplus
namespace egihash
{
	bool test_function();
}

extern "C"
{
#endif // __cplusplus

#define egihash_h256_static_init(...)			\
	{ {__VA_ARGS__} }

#define EGINS_PREFIX egihash
#define EGIHASH_CONCAT(x, y) EGIHASH_CONCAT_(x, y)
#define EGIHASH_CONCAT_(x, y) x ## y
#define EGINS(name) EGINS_(_ ## name)
#define EGINS_(name) EGIHASH_CONCAT(EGINS_PREFIX, name)

typedef int (* EGINS(callback))(unsigned int);
typedef struct EGINS(light) * EGINS(light_t);
typedef struct EGINS(full) * EGINS(full_t);
typedef struct EGINS(h256) { uint8_t b[32]; } EGINS(h256_t);
typedef struct EGINS(result) { EGINS(h256_t) value; EGINS(h256_t) mixhash; } EGINS(result_t);

EGINS(light_t) EGINS(light_new)(unsigned int block_number);
EGINS(result_t) EGINS(light_compute)(EGINS(light_t) light, EGINS(h256_t) header_hash, uint64_t nonce);
void EGINS(light_delete)(EGINS(light_t) light);

EGINS(full_t) EGINS(full_new)(EGINS(light_t) light, EGINS(callback) callback);
uint64_t EGINS(full_dag_size)(EGINS(full_t) full);
void const * EGINS(full_dag)(EGINS(full_t) full);
EGINS(result_t) EGINS(full_compute)(EGINS(full_t) full, EGINS(h256_t) header_hash, uint64_t nonce);
void EGINS(full_delete)(EGINS(full_t) full);
void egihash_h256_compute(EGINS(h256_t) * output_hash, void * input_data, uint64_t input_size);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
