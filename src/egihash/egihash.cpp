// Copyright (c) 2017 Ryan Lucchese
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "egihash.h"
#include "internal.h"
#include "data_sizes.h"
#include <iostream>

extern "C"
{
#include "keccak-tiny.h"
}



namespace egihash
{
	constexpr egihash_h256_t empty_h256 = {{0}};
	constexpr egihash_result_t  empty_result = {{{0}}, {{0}}};
}


extern "C"
{
	struct egihash_light
	{
		unsigned int block_number;
		egihash::VecDeserializedHash512 cache;

		egihash_light(unsigned int block_number)
		: block_number(block_number)
		, cache(egihash::mkcache(egihash::get_cache_size(block_number), egihash::get_seedhash(block_number)))
		{}

		egihash_result_t  compute(egihash_h256_t  header_hash, uint64_t nonce)
		{
			// TODO: copy-free version
			egihash_result_t  result;
			auto ret = egihash::hashimoto_light(egihash::get_full_size(block_number)
			, cache
			, egihash::sha3_256(header_hash).deserialize()
			, nonce);

			auto const & val = ret["result"];
			auto const & mix = ret["mix hash"];
			::std::memcpy(result.value.b, &(*val)[0], sizeof(result.value.b));
			::std::memcpy(result.mixhash.b, &(*mix)[0], sizeof(result.mixhash.b));
			return result;
		}
	};

	struct egihash_full
	{
		egihash_light_t light;
		::std::vector<egihash::DeserializedHash512> dataset;

		egihash_full(egihash_light_t  light, egihash_callback callback)
		: light(light)
		, dataset(egihash::calc_dataset(light->cache, egihash::get_full_size(light->block_number), callback))
		{
		}

		egihash_result_t  compute(egihash_h256_t  header_hash, uint64_t nonce)
		{
			// TODO: copy free version
			// TODO: validate memset sizes i.e. min(sizeof(dest), sizeof(src))
			egihash_result_t  result;
			auto ret = egihash::hashimoto_full(egihash::get_full_size(light->block_number)
			, dataset
			, egihash::sha3_256(header_hash).deserialize()
			, nonce);

			auto const & val = ret["result"];
			auto const & mix = ret["mix hash"];
			::std::memcpy(result.value.b, &(*val)[0], sizeof(result.value.b));
			::std::memcpy(result.mixhash.b, &(*mix)[0], sizeof(result.mixhash.b));
			return result;
		}
	};

	egihash_light_t  egihash_light_new(unsigned int block_number)
	{
		try
		{
			return new egihash_light(block_number);
		}
		catch (...)
		{
			return 0; // nullptr return indicates error
		}
	}

	egihash_result_t  egihash_light_compute(egihash_light_t  light, egihash_h256_t  header_hash, uint64_t nonce)
	{
		try
		{
			return light->compute(header_hash, nonce);
		}
		catch (...)
		{
			return egihash::empty_result; // empty result indicates error
		}
	}

	void egihash_light_delete(egihash_light_t  light)
	{
		try
		{
			delete light;
		}
		catch (...)
		{
			// no way to indicate error
		}
	}

	egihash_full_t  egihash_full_new(egihash_light_t  light, egihash_callback callback)
	{
		try
		{
			return new egihash_full(light, callback);
		}
		catch (...)
		{
			return 0; // nullptr indicates error
		}
	}

	uint64_t egihash_full_dag_size(egihash_full_t  full)
	{
		try
		{
			return egihash::get_full_size(full->light->block_number);
		}
		catch (...)
		{
			return 0; // zero result indicates error
		}
	}

	void const * egihash_full_dag(egihash_full_t  full)
	{
		try
		{
			return &full->dataset[0];
		}
		catch (...)
		{
			return 0; // nullptr indicates error
		}
	}

	egihash_result_t  egihash_full_compute(egihash_full_t  full, egihash_h256_t  header_hash, uint64_t nonce)
	{
		try
		{
			return full->compute(header_hash, nonce);
		}
		catch (...)
		{
			return egihash::empty_result; // empty result indicates error
		}
	}

	void egihash_full_delete(egihash_full_t  full)
	{
		try
		{
			delete full;

		}
		catch (...)
		{
			// no way to indicate error
		}
	}

	uint64_t egihash_get_datasize(uint64_t const block_number)
	{
		assert(block_number / egihash::EPOCH_LENGTH < 2048);
		return dag_sizes[block_number / egihash::EPOCH_LENGTH];
	}

	uint64_t egihash_get_cachesize(uint64_t const block_number)
	{
		assert(block_number / egihash::EPOCH_LENGTH < 2048);
		return cache_sizes[block_number / egihash::EPOCH_LENGTH];
	}



	void egihash_h256_compute(egihash_h256_t  * output_hash, void * input_data, uint64_t input_size)
	{
		try
		{
			egihash::sha3_256 hash(input_data, input_size);
			::std::memcpy(output_hash->b, hash.data, hash.HASH_SIZE);
		}
		catch (...)
		{
			// zero hash data indicates error
			::std::memset(output_hash->b, 0, 32);
		}
	}

	// Returns if hash is less than or equal to boundary (2^256/difficulty)
	bool egihash_check_difficulty(
		egihash_h256_t const* hash,
		egihash_h256_t const* boundary)
	{
		try
		{
			// Boundary is big endian
			for (int i = 0; i < 32; i++)
			{
				if ( egihash::sha3_256(*hash).data[i] == egihash::sha3_256(*boundary).data[i] )
				{
					continue;
				}
				return egihash::sha3_256(*hash).data[i] < egihash::sha3_256(*boundary).data[i];
			}
		}
		catch (...)
		{
			return false;
		}

		return true;
	}

	uint32_t fnv_hash(uint32_t const x, uint32_t const y)
	{
		return egihash::fnv(x, y);
	}
}

