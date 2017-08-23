/*
 * internal.cpp
 *
 *  Created on: Aug 15, 2017
 *      Author: ranjeetdevgun
 */

#include "internal.h"

namespace egihash
{
	template <typename HashType> DeserializedHash<HashType> hash_words(::std::string const & data)
	{
		auto const hash = HashType(data);
		return hash.deserialize();
	}

	template <typename HashType> DeserializedHash<HashType> hash_words(DeserializedHash<HashType> const & deserialized)
	{
		auto const serialized = HashType::serialize(deserialized);
		return hash_words<HashType>(serialized);
	}

	template <typename T> DeserializedHash<sha3_512> sha3_512_deserialized(T const & data)
	{
		return hash_words<sha3_512>(data);
	}
	
	template <typename T> DeserializedHash<sha3_256> sha3_256_deserialized(T const & data)
	{
		return hash_words<sha3_256>(data);
	}

	template <typename T> ::std::string serialize_cache(T const & cache_data)
	{
		::std::string ret;
		for (auto const & i : cache_data)
		{
			ret += serialize_hash(cache_data);
		}
	}

	template <typename T> ::std::string serialize_dataset(T const & dataset)
	{
		return serialize_cache(dataset);
	}

	::std::string get_seedhash(size_t const block_number)
	{
		::std::string s(32, 0);
		for (size_t i = 0; i < (block_number / EPOCH_LENGTH); i++)
		{
			s = sha3_256::serialize(sha3_256_deserialized(s));
		}
		return s;
	}

	VecDeserializedHash512 mkcache(size_t const cache_size, ::std::string seed)
	{
		size_t n = cache_size / HASH_BYTES;

		VecDeserializedHash512 o{sha3_512_deserialized(seed)};
		for (size_t i = 1; i < n; i++)
		{
			o.push_back(sha3_512_deserialized(o.back()));
		}

		for (size_t i = 0; i < CACHE_ROUNDS; i++)
		{
			for (size_t j = 0; j < n; j++)
			{
				auto v = o[j][0] % n;
				auto & u = o[(j-1+n)%n];
				size_t count = 0;
				for (auto & k : u)
				{
					count++;
					k = k ^ o[v][count];
				}
				o[i] = sha3_512_deserialized(u);
			}
		}

		return o;
	}

	DeserializedHash512 calc_dataset_item(VecDeserializedHash512 const & cache, size_t const i)
	{
		size_t const n = cache.size();
		constexpr size_t r = HASH_BYTES / WORD_BYTES;
		DeserializedHash512 mix(cache[i%n].size());
		std::copy(cache[i%n].begin(), cache[i%n].end(), mix.begin());
		mix[0] ^= i;
		mix = sha3_512_deserialized(mix);
		for (size_t j = 0; j < DATASET_PARENTS; j++)
		{
			size_t const cache_index = fnv(i ^ j, mix[j % r]);
			auto l = cache[cache_index % n].begin();
			auto lEnd = cache[cache_index % n].end();
			for (auto k = mix.begin(), kEnd = mix.end();
				((k != kEnd) && (l != lEnd)); k++, l++)
			{
				*k = fnv(*k, *l);
			}

		}
		return sha3_512_deserialized(mix);
	}

	VecDeserializedHash512 calc_dataset(VecDeserializedHash512 const & cache, size_t const full_size, egihash_callback progress_callback)
	{
		VecDeserializedHash512 out;
		for (size_t i = 0; i < (full_size / HASH_BYTES); i++)
		{
			out.push_back(calc_dataset_item(cache, i));
			if (progress_callback(i) != 0)
			{
				throw hash_exception("DAG creation cancelled.");
			}
		}
		return out;
	}

	HashimotoResult hashimoto(DeserializedHash256 const & header, uint64_t const nonce, size_t const full_size, Lookup lookup)
	{
		auto const n = full_size / HASH_BYTES;
		auto const w = MIX_BYTES / WORD_BYTES;
		auto const mixhashes = MIX_BYTES / HASH_BYTES;

		DeserializedHash256 header_seed(header);
		for (size_t i = 0; i < 8; i++)
		{
			// TODO: nonce is big endian, this converts to little endian (do something sensible for big endian)
			header_seed.push_back(reinterpret_cast<uint8_t const *>(&nonce)[7 - i]);
		}
		
		auto s = sha3_512_deserialized(header_seed);
		decltype(s) mix;
		for (size_t i = 0; i < (MIX_BYTES / HASH_BYTES); i++)
		{
			mix.insert(mix.end(), s.begin(), s.end());
		}

		for (size_t i = 0; i < ACCESSES; i++)
		{
			auto p = fnv(i ^ s[0], mix[i % w]) % (n / mixhashes) * mixhashes;
			decltype(s) newdata;
			for (size_t j = 0; j < (MIX_BYTES / HASH_BYTES); j++)
			{
				auto const & h = lookup(p + j);
				newdata.insert(newdata.end(), h.begin(), h.end());
			}
			for (auto j = mix.begin(), jEnd = mix.end(), k = newdata.begin(), kEnd = newdata.end(); j != jEnd && k != kEnd; j++, k++)
			{
				*j = fnv(*j, *k);
			}
		}

		decltype(s) cmix;
		for (size_t i = 0; i < mix.size(); i += 4)
		{
			cmix.push_back(fnv(fnv(fnv(mix[i], mix[i+1]), mix[i+2]), mix[i+3]));
		}

		::std::shared_ptr<decltype(s)> shared_mix(::std::make_shared<decltype(s)>(std::move(cmix)));
		::std::map<::std::string, decltype(shared_mix)> out;
		out.insert(decltype(out)::value_type(::std::string("mix hash"), shared_mix));
		s.insert(s.end(), shared_mix->begin(), shared_mix->end());
		out.insert(decltype(out)::value_type(::std::string("result"), ::std::make_shared<decltype(s)>(sha3_256_deserialized(s))));
		return out;
	}

	HashimotoResult hashimoto_light(size_t const full_size, VecDeserializedHash512 const cache, DeserializedHash256 const & header, uint64_t const nonce)
	{
		return hashimoto(header, nonce, full_size, [cache](size_t const x){return calc_dataset_item(cache, x);});
	}

	HashimotoResult hashimoto_full(size_t const full_size, VecDeserializedHash512 const dataset, DeserializedHash256 const & header, uint64_t const nonce)
	{
		return hashimoto(header, nonce, full_size, [dataset](size_t const x){return dataset[x];});
	}	
}

