// Copyright (c) 2017 Ryan Lucchese
// Distributed under the MIT software license, see 
// http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
namespace egihash
{
    bool test_function();
}

extern "C"
{
#endif // __cplusplus

#define EGIHASH_NAMESPACE_PREFIX egihash
#define EGIHASH_CONCAT(x, y) EGIHASH_CONCAT_(x, y)
#define EGIHASH_CONCAT_(x, y) x ## y
#define EGIHASH_NAMESPACE(name) EGIHASH_NAMESPACE_(_ ## name)
#define EGIHASH_NAMESPACE_(name) EGIHASH_CONCAT(EGIHASH_NAMESPACE_PREFIX, name)

typedef int (* EGIHASH_NAMESPACE(callback))(unsigned int);
typedef struct EGIHASH_NAMESPACE(light) * EGIHASH_NAMESPACE(light_t);
typedef struct EGIHASH_NAMESPACE(full) * EGIHASH_NAMESPACE(full_t);
typedef struct EGIHASH_NAMESPACE(h256) { uint8_t b[32]; } EGIHASH_NAMESPACE(h256_t);
typedef struct EGIHASH_NAMESPACE(result) { EGIHASH_NAMESPACE(h256_t) value; EGIHASH_NAMESPACE(h256_t) mixhash; } EGIHASH_NAMESPACE(result_t);

EGIHASH_NAMESPACE(light_t) EGIHASH_NAMESPACE(light_new)(unsigned int block_number);
EGIHASH_NAMESPACE(result_t) EGIHASH_NAMESPACE(light_compute)(EGIHASH_NAMESPACE(light_t) light, EGIHASH_NAMESPACE(h256_t) header_hash, uint64_t nonce);
void EGIHASH_NAMESPACE(light_delete)(EGIHASH_NAMESPACE(light_t) light);

EGIHASH_NAMESPACE(full_t) EGIHASH_NAMESPACE(full_new)(EGIHASH_NAMESPACE(light_t) light, EGIHASH_NAMESPACE(callback) callback);
uint64_t EGIHASH_NAMESPACE(full_dag_size)(EGIHASH_NAMESPACE(full_t) full);
void const * EGIHASH_NAMESPACE(full_dag)(EGIHASH_NAMESPACE(full_t) full);
EGIHASH_NAMESPACE(result_t) EGIHASH_NAMESPACE(full_compute)(EGIHASH_NAMESPACE(full_t) full, EGIHASH_NAMESPACE(h256_t) header_hash, uint64_t nonce);
void EGIHASH_NAMESPACE(full_delete)(EGIHASH_NAMESPACE(full_t) full);

void egihash_h256_compute(EGIHASH_NAMESPACE(h256_t) * output_hash, void * input_data, uint64_t input_size);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus