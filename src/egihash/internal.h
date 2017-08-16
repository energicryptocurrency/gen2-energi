/*
 * internal.h
 *
 *  Created on: Aug 15, 2017
 *      Author: ranjeetdevgun
 */

#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>
#include <type_traits>
#include <cassert>

extern "C" {
#include "secure_memzero.h"
#include "keccak-tiny.h"
}
#include "egihash.h"

namespace egihash
{
	namespace 
	{
		constexpr uint32_t WORD_BYTES = 4u;                       // bytes in word
		constexpr uint32_t DATASET_BYTES_INIT = 1u << 30u;        // bytes in dataset at genesis
		constexpr uint32_t DATASET_BYTES_GROWTH = 1u << 23u;      // dataset growth per epoch
		constexpr uint32_t CACHE_BYTES_INIT = 1u << 24u;          // bytes in cache at genesis
		constexpr uint32_t CACHE_BYTES_GROWTH = 1u << 17u;        // cache growth per epoch
		constexpr uint32_t CACHE_MULTIPLIER=1024u;                // Size of the DAG relative to the cache
		constexpr uint32_t EPOCH_LENGTH = 30000u;                 // blocks per epoch
		constexpr uint32_t MIX_BYTES = 128u;                      // width of mix
		constexpr uint32_t HASH_BYTES = 64u;                      // hash length in bytes
		constexpr uint32_t DATASET_PARENTS = 256u;                // number of parents of each dataset element
		constexpr uint32_t CACHE_ROUNDS = 3u;                     // number of rounds in cache production
		constexpr uint32_t ACCESSES = 64u;                        // number of accesses in hashimoto loop
	}
	
	
	template <typename IntegralType >
	typename ::std::enable_if<::std::is_integral<IntegralType>::value, bool>::type
	/*bool*/ is_prime(IntegralType x) noexcept
	{
		for (auto i = IntegralType(2); i <= ::std::sqrt(x); i++)
		{
			if ((x % i) == 0) return false;
		}
		return true;
	}

	uint64_t inline get_cache_size(uint64_t block_number) noexcept
	{
		uint64_t cache_size = (CACHE_BYTES_INIT + (CACHE_BYTES_GROWTH * (block_number / EPOCH_LENGTH))) - HASH_BYTES;
		while (!is_prime(cache_size / HASH_BYTES))
		{
			cache_size -= (2 * HASH_BYTES);
		}
		return cache_size;
	}

	uint64_t inline get_full_size(uint64_t block_number) noexcept
	{
		uint64_t full_size = (DATASET_BYTES_INIT + (DATASET_BYTES_GROWTH * (block_number / EPOCH_LENGTH))) - MIX_BYTES;
		while (!is_prime(full_size / MIX_BYTES))
		{
			full_size -= (2 * MIX_BYTES);
		}
		return full_size;
	}


	inline int32_t decode_int(uint8_t const * data, uint8_t const * dataEnd) noexcept
	{
		if (!data || (dataEnd < (data + 3)))
			return 0;

		return static_cast<int32_t>(
			(static_cast<int32_t>(data[0]) << 24) |
			(static_cast<int32_t>(data[1]) << 16) |
			(static_cast<int32_t>(data[2]) << 8) |
			(static_cast<int32_t>(data[3]))
		);
	}

	inline ::std::string zpad(::std::string const & str, size_t const length)
	{
		return str + ::std::string(::std::max(length - str.length(), static_cast<::std::string::size_type>(0)), 0);
	}


	template <typename IntegralType >
	typename ::std::enable_if<::std::is_integral<IntegralType>::value, ::std::string>::type
	/*::std::string*/ encode_int(IntegralType x)
	{
		using namespace std;

		if (x == 0) return string();

		// TODO: fast hex conversion
		stringstream ss;
		ss << hex << x;
		string hex_str = ss.str();
		string encoded(hex_str.length() % 2, '0');
		encoded += hex_str;

		string ret;
		ss.str(string());
		for (size_t i = 0; i < encoded.size(); i += 2)
		{
			ret += static_cast<char>(stoi(encoded.substr(i, 2), 0, 16));
		}

		return ret;
	}
	
	namespace
	{
		static constexpr uint32_t FNV_PRIME = 0x01000193ull;             // prime number used for FNV hash function
		static constexpr uint64_t FNV_MODULUS = 1ull << 32ull;           // modulus used for FNV hash function
		constexpr inline uint32_t fnv (uint32_t v1, uint32_t v2) noexcept
		{
			return ((v1 * FNV_PRIME) ^ v2) % FNV_MODULUS;
		}
	};
	
	class hash_exception : public ::std::runtime_error
	{
	public:
		hash_exception(std::string const & what_arg) noexcept
		: runtime_error(what_arg)
		{}

		hash_exception(char const * what_arg) noexcept
		: runtime_error(what_arg)
		{}
	};
	
	using HashFunction = int (*)(uint8_t *, size_t, uint8_t const * in, size_t);
	template <size_t HashSize, HashFunction HashF>
	struct sha3_base
	{
		using deserialized_hash = ::std::vector<int32_t>;
		static constexpr size_t HASH_SIZE = HashSize;
		uint8_t data[HASH_SIZE];

		sha3_base(sha3_base const &) = default;
		sha3_base(sha3_base &&) = default;
		sha3_base & operator=(sha3_base const &) = default;
		sha3_base & operator=(sha3_base &&) = default;
		~sha3_base() = default;

		sha3_base()
		: data{0}
		{}

		sha3_base(::std::string const & input)
		: data{0}
		{
			compute_hash(input.c_str(), input.size());
		}

		sha3_base(void const * input, size_t const input_size)
		: data{0}
		{
			compute_hash(input, input_size);
		}

		void compute_hash(void const * input, size_t const input_size)
		{
			if (HashF(data, HASH_SIZE, reinterpret_cast<uint8_t const *>(input), input_size) != 0)
			{
				throw hash_exception("Unable to compute hash"); // TODO: better message?
			}
		}

		deserialized_hash deserialize() const
		{
			deserialized_hash out(HASH_SIZE / 4, 0);
			for (size_t i = 0, j = 0; i < HASH_SIZE; i += WORD_BYTES, j++)
			{
				out[j] = decode_int(&data[i], &data[HASH_SIZE - 1]);
			}
			return out;
		}

		static ::std::string serialize(deserialized_hash const & h)
		{
			::std::string ret;
			for (auto const i : h)
			{
				ret += zpad(encode_int(i), 4);
			}
			return ret;
		}

		operator ::std::string() const
		{
			// TODO: fast hex conversion
			::std::stringstream ss;
			ss << ::std::hex;
			for (auto const i : data)
			{
				ss << ::std::setw(2) << ::std::setfill('0') << static_cast<uint32_t>(i);
			}
			return ss.str();
		}
	};

	struct sha3_256 : public sha3_base<32, ::sha3_256>
	{
		sha3_256(::std::string const & input)
		: sha3_base(input)
		{}

		sha3_256(void const * input, size_t const input_size)
		: sha3_base(input, input_size)
		{}

		sha3_256(egihash_h256_t  const & h256)
		: sha3_base()
		{
			::std::memcpy(&data[0], &h256.b[0], HASH_SIZE);
		}
	};

	struct sha3_512 : public sha3_base<64, ::sha3_512>
	{
		sha3_512(::std::string const & input)
		: sha3_base(input)
		{

		}

		sha3_512(void const * input, size_t const input_size)
		: sha3_base(input, input_size)
		{

		}
	};

	template <typename HashType> using DeserializedHash = typename HashType::deserialized_hash;
	using DeserializedHash256 = DeserializedHash<sha3_256>;
	using DeserializedHash512 = DeserializedHash<sha3_512>;
	using VecDeserializedHash256 = ::std::vector<DeserializedHash256>;
	using VecDeserializedHash512 = ::std::vector<DeserializedHash512>;

	template <typename HashType> DeserializedHash<HashType> hash_words(::std::string const & data);
	template <typename HashType> DeserializedHash<HashType> hash_words(DeserializedHash<HashType> const & deserialized);

	template <typename T> DeserializedHash<sha3_512> sha3_512_deserialized(T const & data);
	template <typename T> DeserializedHash<sha3_256> sha3_256_deserialized(T const & data);

	template <typename T> ::std::string serialize_cache(T const & cache_data);
	template <typename T> ::std::string serialize_dataset(T const & dataset);
	::std::string get_seedhash(size_t const block_number);

	VecDeserializedHash512 mkcache(size_t const cache_size, ::std::string seed);

	DeserializedHash512 calc_dataset_item(VecDeserializedHash512 const & cache, size_t const i);
	VecDeserializedHash512 calc_dataset(VecDeserializedHash512 const & cache, size_t const full_size, egihash_callback progress_callback);

	using Lookup = ::std::function<DeserializedHash512 (size_t const)>;
	using HashimotoResult = ::std::map<::std::string, ::std::shared_ptr<DeserializedHash512>>;
	HashimotoResult hashimoto(DeserializedHash256 const & header, uint64_t const nonce, size_t const full_size, Lookup lookup);
	HashimotoResult hashimoto_light(size_t const full_size, VecDeserializedHash512 const cache, DeserializedHash256 const & header, uint64_t const nonce);
	HashimotoResult hashimoto_full(size_t const full_size, VecDeserializedHash512 const dataset, DeserializedHash256 const & header, uint64_t const nonce);
}


