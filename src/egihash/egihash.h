// Copyright (c) 2017 Ryan Lucchese
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>
extern "C" {
#include "secure_memzero.h"
#include "keccak-tiny.h"
}

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#define egihash_h256_static_init(...)			\
	{ {__VA_ARGS__} }

typedef int (*egihash_callback)(unsigned int);
typedef struct egihash_light* egihash_light_t;
typedef struct egihash_full* egihash_full_t;
typedef struct egihash_h256 { uint8_t b[32]; } egihash_h256_t;
typedef struct egihash_result { egihash_h256_t value; egihash_h256_t mixhash; } egihash_result_t;

egihash_light_t egihash_light_new(unsigned int block_number);
void egihash_light_delete(egihash_light_t light);
egihash_result_t egihash_light_compute(egihash_light_t light, egihash_h256_t header_hash, uint64_t nonce);

egihash_full_t egihash_full_new(egihash_light_t light, egihash_callback callback);
void egihash_full_delete (egihash_full_t full);
uint64_t egihash_full_dag_size(egihash_full_t full);
void const * egihash_full_dag(egihash_full_t full);
egihash_result_t egihash_full_compute(egihash_full_t full, egihash_h256_t header_hash, uint64_t nonce);

void egihash_h256_compute(egihash_h256_t * output_hash, void * input_data, uint64_t input_size);

uint32_t fnv_hash(uint32_t const x, uint32_t const y);
bool egihash_check_difficulty(
		egihash_h256_t const* hash,
		egihash_h256_t const* boundary);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
