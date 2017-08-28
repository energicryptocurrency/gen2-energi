// Copyright (c) 2017 Ryan Lucchese
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>

#ifdef __cplusplus

#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace egihash
{
	bool test_function() noexcept;

	namespace constants
	{
		/** \brief DAG_MAGIC_BYTES is the starting sequence of a DAG file, used for identification.
		*/
		static constexpr char DAG_MAGIC_BYTES[] = "EGIHASH_DAG";

		/** \brief DAG_FILE_HEADER_SIZE is the expected size of a DAG file header.
		*/
		static constexpr uint32_t DAG_FILE_HEADER_SIZE = 64u;

		/** \brief DAG_FILE_MINIMUM_SIZE is the size of the DAG file at epoch 0.
		*/
		static constexpr uint64_t DAG_FILE_MINIMUM_SIZE = 1090516864;

		/** \brief CALLBACK_FREQUENCY determines how often callbacks will be called.
		*
		*	1 means every iteration, 10 means every 10th iteration, and so on...
		*/
		static constexpr uint32_t CALLBACK_FREQUENCY = 1024u;

		/** \brief The major version of egihash
		*/
		static constexpr uint32_t MAJOR_VERSION = 1u;

		/** \brief The revision number (middle version digit) of egihash
		*/
		static constexpr uint32_t REVISION = 23u;

		/** \brief The minor version number of egihash
		*/
		static constexpr uint32_t MINOR_VERSION = 0u;

		/** \brief Number of bytes in a hash word.
		*/
		static constexpr uint32_t WORD_BYTES = 4u;

		/** \brief The number of bytes in the dataset at genesis.
		*/
		static constexpr uint32_t DATASET_BYTES_INIT = 1u << 30u;

		/** \brief The growth of the dataset in bytes per epoch.
		*/
		static constexpr uint32_t DATASET_BYTES_GROWTH = 1u << 23u;

		/** \brief The number of bytes in the cache at genesis.
		*/
		static constexpr uint32_t CACHE_BYTES_INIT = 1u << 24u;

		/** \brief The growth of the cache in bytes per epoch.
		*/
		static constexpr uint32_t CACHE_BYTES_GROWTH = 1u << 17u;

		/** \brief Ratio of the size of the DAG in bytes relative to the size of the cache in bytes..
		*/
		static constexpr uint32_t CACHE_MULTIPLIER=1024u;

		/** \brief The number of blocks which constitute one epoch.
		*
		*	The DAG and cache must be regenerated once per epoch.
		*/
		static constexpr uint32_t EPOCH_LENGTH = 30000u;

		/** \brief The width of the mix hash for egihash.
		*/
		static constexpr uint32_t MIX_BYTES = 128u;

		/** \brief The size of an egihash in bytes.
		*/
		static constexpr uint32_t HASH_BYTES = 64u;

		/** \brief The number of parents for each element in the DAG.
		*/
		static constexpr uint32_t DATASET_PARENTS = 256u;

		/** \brief The number of hashing rounds used to produce the cache.
		*/
		static constexpr uint32_t CACHE_ROUNDS = 3u;

		/** \brief The number of DAG lookups to compute an egihash.
		*/
		static constexpr uint32_t ACCESSES = 64u;

		// TODO: C API
		//static constexpr EGIHASH_NAMESPACE(h256_t) empty_h256 = {{0}};
		//static constexpr EGIHASH_NAMESPACE(result_t) empty_result = {{{0}}, {{0}}};
	}

	/** \brief hash_exception indicates an error or cancellation when performing a task within egihash.
	*
	*	All functions not marked noexcept may be assumed to throw hash_exception or C++ runtime exceptions.
	*/
	class hash_exception : public ::std::runtime_error
	{
	public:
		/** \brief construct a hash_exception from a std::string const &
		*/
		hash_exception(std::string const & what_arg) noexcept
		: runtime_error(what_arg)
		{

		}

		/** \brief construct a hash_exception from a char const *
		*/
		hash_exception(char const * what_arg) noexcept
		: runtime_error(what_arg)
		{

		}
	};

	/** \brief epoch0_seedhash is is the seed hash for the genesis block and first epoch of the DAG.
	*			All seed hashes for subsequent epochs will be generated from this seedhash.
	*
	*	The epoch0_seedhash should be set to a randomized set of 32 bytes for a given crypto currency.
	*	This represents a keccak-256 hash that will be used as input for building the DAG/cache.
	*/
	// TODO: randomized seedhash not zero seedhash
	static constexpr char epoch0_seedhash[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
	static_assert(sizeof(epoch0_seedhash) == 33, "Invalid seedhash");

	/** \brief get_seedhash(uint64_t) will compute the seedhash for a given block number.
	*
	*	\param block_number An unsigned 64-bit integer representing the block number for which to compute the seed hash.
	*	\return A hexidecimal encoded keccak-256 seed hash for the given block number.
	*/
	::std::string get_seedhash(uint64_t const block_number);

	/** \brief h256_t represents a the result of a keccak-256 hash.
	*/
	struct h256_t
	{
		using size_type = ::std::size_t;
		static constexpr size_type hash_size = 32;

		/** \brief Default constructor for h256_t fills data field b with 0 bytes
		*/
		constexpr h256_t(): b{0} {}

		/** \brief Default copy constructor
		*/
		h256_t(h256_t const &) = default;

		/** \brief Default copy assignment operator
		*/
		h256_t & operator=(h256_t const &) = default;

		/** \brief Default move constructor
		*/
		h256_t(h256_t &&) = default;

		/** \brief Default move assignment operator
		*/
		h256_t & operator=(h256_t &&) = default;

		/** \brief Default destructor
		*/
		~h256_t() = default;

		/** \brief Construct and compute a hash from a data source.
		*
		*	\param input_data A pointer to the start of the data to be hashed
		*	\param input_size The number of bytes of input data to hash
		*	\throws hash_exception on error
		*/
		h256_t(void const * input_data, size_type input_size);

		/** \brief Test if this hash is valid. Returns true if hash data is not all 0 bytes.
		*/
		operator bool() const;

		/** \brief This member stores the 256-bit hash data
		*/
		uint8_t b[hash_size];
	};

	/** \brief A default constructed h256_t has all empty bytes.
	*/
	static constexpr h256_t empty_h256;

	/** \brief result_t represents the result of an egihash.
	*/
	struct result_t
	{
		/** \brief Default constructor
		*/
		constexpr result_t() = default;

		/** \brief Default copy constructor
		*/
		result_t(result_t const &) = default;

		/** \brief Default copy assignment operator
		*/
		result_t & operator=(result_t const &) = default;

		/** \brief Default move constructor
		*/
		result_t(result_t &&) = default;

		/** \brief Default move assignment operator
		*/
		result_t & operator=(result_t &&) = default;

		/** \brief Default destructor
		*/
		~result_t() = default;

		/** \brief Test if this hash is valid. Returs true of hash data is not all 0 bytes.
		*/
		operator bool() const;

		/** \brief This member contains the egihash result value.
		*/
		h256_t value;

		/** \brief This member contains the mix hash to produce this result.
		*/
		h256_t mixhash;
	};

	/** \brief A default constructed result_t has all empty bytes.
	*/
	static constexpr result_t empty_result;

	/** \brief progress_callback_phase values represent different stages at which a progress callback may be called.
	*/
	enum progress_callback_phase
	{
		cache_seeding,		/**< cache_seeding means fill the cache with hashes of seed hashes: { cache_0 = seedhash; cache_n = keccak512(cache_n-1) } */
		cache_generation,	/**< cache_generation is performed with a memory hard hashing function of a seeded cache */
		cache_saving,		/**< cache_saving is saving the cache to disk */
		cache_loading,		/**< cache_loading is loading the cache from disk */
		dag_generation,		/**< dag_generation is computing the DAG for a given epoch (block_number) */
		dag_saving,			/**< dag_saving is saving the DAG to disk */
		dag_loading			/**< dag_loading is loading the DAG from disk */
	};

	/** \brief progress_callback_type is a function which may be passed to any phase of DAG/cache or generation to receive progress updates.
	*
	*	\param step is the count of the step just compeleted before this call of the callback.
	*	\param max is the maximum number of steps that will be performed for this progress_callback_phase.
	*	\param phase the progress_callback_phase indicating what is being computed at the time of the callback.
	*	\return false to cancel whatever action is being performed, true to continue.
	*/
	using progress_callback_type = ::std::function<bool (::std::size_t step, ::std::size_t max, progress_callback_phase phase)>;

	/** \brief read_function_type is a function which passed to various objects which perform loading of a file, such as the cache and DAG.
	*
	*	Note that this function will own whatever data it needs to perform the read, i.e. the filestream.
	*	\param dst points to the memory location that should be read into.
	*	\param count represents the number of bytes to read.
	*	\throws hash_exception if the read failed
	*/
	using read_function_type = ::std::function<void(void * dst, ::std::size_t count)>;

	/** \brief cache_t is the cache used to compute a DAG for a given epoch.
	*
	* Each DAG owns a cache_t and the size of the cache grows linearly in time.
	*/
	struct cache_t
	{
		/** \brief size_type represents sizes used by a cache.
		*/
		using size_type = ::std::size_t;

		/** \brief data_type is the underlying data store which stores a cache.
		*/
		using data_type = ::std::vector<::std::vector<int32_t>>;

		/** \brief default copy constructor.
		*/
		cache_t(const cache_t &) = default;

		/** \brief default copy assignment operator.
		*/
		cache_t & operator=(cache_t const &) = default;

		/** \brief default move constructor.
		*/
		cache_t(cache_t &&) = default;

		/** \brief default move assignment operator.
		*/
		cache_t & operator=(cache_t &&) = default;

		/** \brief default destructor.
		*/
		~cache_t() = default;

		/** \brief explicitly deleted default constructor.
		*/
		cache_t() = delete;

		/** \brief Construct a cache_t given a block number, a seed hash, and a progress callback function.
		*
		*	\param block_number is the block number for which this cache_t is to be constructed.
		*	\param seed is the seed hash (i.e. get_seedhash(block_number)) for which this cache_t is to be constructed.
		*	\param callback (optional) may be used to monitor the progress of cache generation. Return false to cancel, true to continue.
		*/
		cache_t(uint64_t block_number, ::std::string const & seed, progress_callback_type callback = [](size_type, size_type, int){ return true; });

		/** \brief Get the epoch number for which this cache is valid.
		*
		*	\returns uint64_t representing the epoch number (block_number / constants::EPOCH_LENGTH)
		*/
		uint64_t epoch() const;

		/** \brief Get the size of the cache data in bytes.
		*
		*	\returns size_type representing the size of the cache data in bytes.
		*/
		size_type size() const;

		/** \brief Get the data the cache contains.
		*
		*	\returns data_type const & to the actual cache data.
		*/
		data_type const & data() const;

		/** \brief Get the size of the cache data in bytes.
		*
		*	\param block_number is the block number for which cache size to compute.
		*	\returns size_type representing the size of the cache data in bytes.
		*/
		static size_type get_cache_size(uint64_t const block_number) noexcept;

	private:
		friend struct dag_t;

		/** \brief Construct a cache_t by loading from disk.
		*
		*	\param epoch is the number of the epoch for the cache we are loading.
		*	\param size is the size in bytes of the cache we are loading.
		*	\param read A function which will read cache data from disk.
		*	\param callback (optional) may be used to monitor the progress of cache loading. Return false to cancel, true to continue.
		*/
		cache_t(uint64_t epoch, uint64_t size, read_function_type read, progress_callback_type callback = [](size_type, size_type, int){ return true; });

		/** \brief Load a cache from disk.
		*
		*	\param read A function which will read cache data from disk.
		*	\param callback (optional) may be used to monitor the progress of cache loading. Return false to cancel, true to continue.
		*/
		void load(read_function_type read, progress_callback_type callback = [](size_type, size_type, int){ return true; });

		/** \brief cache_t private implementation.
		*/
		struct impl_t;

		/** \brief shared_ptr to impl allows default moving/copying of cache. Internally, only one cache_t::impl_t per epoch will exist.
		*/
		::std::shared_ptr<impl_t> impl;
	};

	/** \brief dag_t is the DAG which is used by full nodes and miners to compute egihashes.
	*
	*	The DAG gives egihash it's ASIC resistance, ensuring that this hashing function is memory bound not compute bound.
	*	The DAG must be updated once per constants::EPOCH_LENGTH block numbers.
	*	The DAG for epoch 0 is 1073739904 bytes in size and will grow linearly with each following epoch.
	*	The DAG can take a long time to generate. It is recommended to save the DAG to disk to avoid having to regenerate it each time.
	*	It makes sense to pre-compute the DAG for the next epoch, so generating it does not interrupt mining / operation of a full node.
	*	Whether by generation or by loading, the DAG will own a cache_t which corresponds to cache for the same epoch.
	*/
	struct dag_t
	{
		/** \brief size_type represents sizes used by a cache.
		*/
		using size_type = ::std::size_t;

		/** \brief data_type is the underlying data store which stores a cache.
		*/
		using data_type = ::std::vector<::std::vector<int32_t>>;

		/** \brief default copy constructor.
		*/
		dag_t(dag_t const &) = default;

		/** \brief default copy assignment operator.
		*/
		dag_t & operator=(dag_t const &) = default;

		/** \brief default move constructor.
		*/
		dag_t(dag_t &&) = default;

		/** \brief default move assignment operator.
		*/
		dag_t & operator=(dag_t &&) = default;

		/** \brief default destructor.
		*/
		~dag_t() = default;

		/** \brief explicitly deleted default constructor.
		*/
		dag_t() = delete;

		/** \brief generate a DAG for a given block_number.
		*
		*	DAG's are cached in a singleton per epoch. If this DAG is already loaded in memory it will be returned quickly.
		*	If this DAG is not yet loaded, this will take a long time.
		*	\param block_number is the block number for which to generate a DAG.
		*	\param callback (optional) may be used to monitor the progress of DAG generation. Return false to cancel, true to continue.
		*/
		dag_t(uint64_t const block_number, progress_callback_type = [](size_type, size_type, int){ return true; });

		/** \brief load a DAG from a file.
		*
		*	DAG's are cached in a singleton per epoch. If this DAG is already loaded in memory it will be returned quickly.
		*	\param file_path is the path to the file the DAG should be loaded from.
		*	\param callback (optional) may be used to monitor the progress of DAG loading. Return false to cancel, true to continue.
		*/
		dag_t(::std::string const & file_path, progress_callback_type = [](size_type, size_type, int){ return true; });

		/** \brief Get the epoch number for which this DAG is valid.
		*
		*	\returns uint64_t representing the epoch number (block_number / constants::EPOCH_LENGTH)
		*/
		uint64_t epoch() const;

		/** \brief Get the size of the DAG data in bytes.
		*
		*	\returns size_type representing the size of the DAG data in bytes.
		*/
		size_type size() const;

		/** \brief Get the data the DAG contains.
		*
		*	\returns data_type const & to the actual DAG data.
		*/
		data_type const & data() const;

		/** \brief Save the DAG to a file fur future loading.
		*
		*	\param file_path is the path to the file the DAG should be saved to.
		*	\param callback (optional) may be used to monitor the progress of DAG saving. Return false to cancel, true to continue.
		*/
		void save(::std::string const & file_path, progress_callback_type callback = [](size_type, size_type, int){ return true; }) const;

		/** \brief Get the cache for this DAG.
		*
		*	\return cache_t of the cache for this DAG.
		*/
		cache_t get_cache() const;

		/** \brief Unload a DAG.
		*
		*	To actually free a DAG from memory, call this function on a DAG. The DAG will then be released from the internal cache.
		*	Once all references to the DAG for this epoch are destroyed, it will be freed.
		*/
		void unload() const;

		/** \brief Get the size of the DAG data in bytes.
		*
		*	\param block_number is the block number for which DAG size to compute.
		*	\return size_type representing the size of the DAG data in bytes.
		*/
		static size_type get_full_size(uint64_t const block_number) noexcept;

		/** \brief dag_t private implementation.
		*/
		struct impl_t;

		/** \brief shared_ptr to impl allows default moving/copying of cache. Internally, only one dag_t::impl_t per epoch will exist.
		*
		*	Since DAGs consume a large amount of memory, it is important that they are cached.
		*/
		::std::shared_ptr<impl_t> impl;
	};

	namespace full
	{
		/** \brief The full Egihash function to be used by full nodes and miners.
		*
		*	\param dag A const reference to the DAG for the current epoch
		*	\param input_data A pointer to the start of the data to be hashed
		*	\param input_size The number of bytes of input data to hash
		*	\throws hash_exception on error
		*	\return result_t containing hashed data
		*/
		result_t hash(dag_t const & dag, void const * input_data, dag_t::size_type input_size);

		/** \brief The full Egihash function to be used by full nodes and miners.
		*
		*	\param dag A const reference to the DAG for the current epoch.
		*	\param start_ptr A pointer to the start of the data to be hashed
		*	\param end_ptr A pointer to the end of the data to be hashed
		*	\throws hash_exception on error
		*	\return result_t containing hashed data
		*/
		template <typename ptr_t>
		typename ::std::enable_if<::std::is_pointer<ptr_t>::value, result_t>::type
		/*result_t*/ hash(dag_t const & dag, ptr_t const start_ptr, ptr_t const end_ptr)
		{
			return hash(dag, start_ptr, static_cast<dag_t::size_type>((end_ptr - start_ptr) * sizeof(start_ptr[0])));
		}

		/** \brief The full Egihash function to be used by full nodes and miners.
		*
		*	\param dag A const reference to the DAG for the current epoch
		*	\param header_hash A h256_t (Keccak-256) hash of the truncated block header
		*	\param nonce An unsigned 64-bit integer stored in little endian byte order
		*	\throws hash_exception on error
		*	\return result_t containing hashed data
		*/
		result_t hash(dag_t const & dag, h256_t const & header_hash, uint64_t const nonce);
	}

	namespace light
	{
		/** \brief The light Egihash function to be used by light wallets & light verification clients.
		*
		*	\param cache A const reference to the cache for the current epoch
		*	\param input_data A pointer to the start of the data to be hashed
		*	\param input_size The number of bytes of input data to hash
		*	\throws hash_exception on error
		*	\return result_t containing hashed data
		*/
		result_t hash(cache_t const & cache, void const * input_data, cache_t::size_type input_size);

		/** \brief The light Egihash function to be used by light wallets & light verification clients.
		*
		*	\param cache A const reference to the cache for the current epoch.
		*	\param start_ptr A pointer to the start of the data to be hashed
		*	\param end_ptr A pointer to the end of the data to be hashed
		*	\throws hash_exception on error
		*	\return result_t containing hashed data
		*/
		template <typename ptr_t>
		typename ::std::enable_if<::std::is_pointer<ptr_t>::value, result_t>::type
		/*result_t*/ hash(cache_t const & cache, ptr_t const start_ptr, ptr_t const end_ptr)
		{
			return hash(cache, start_ptr, static_cast<cache_t::size_type>((end_ptr - start_ptr) * sizeof(start_ptr[0])));
		}

		/** \brief The light Egihash function to be used by light wallets & light verification clients.
		*
		*	\param dag A const reference to the DAG for the current epoch
		*	\param header_hash A h256_t (Keccak-256) hash of the truncated block header
		*	\param nonce An unsigned 64-bit integer stored in little endian byte order
		*	\throws hash_exception on error
		*	\return result_t containing hashed data
		*/
		result_t hash(cache_t const & cache, h256_t const & header_hash, uint64_t const nonce);
	}
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
typedef struct EGIHASH_NAMESPACE(h512) { uint8_t b[64]; } EGIHASH_NAMESPACE(h512_t);

#if 0 // TODO: FIXME
EGIHASH_NAMESPACE(light_t) EGIHASH_NAMESPACE(light_new)(unsigned int block_number);
EGIHASH_NAMESPACE(result_t) EGIHASH_NAMESPACE(light_compute)(EGIHASH_NAMESPACE(light_t) light, EGIHASH_NAMESPACE(h256_t) header_hash, uint64_t nonce);
void EGIHASH_NAMESPACE(light_delete)(EGIHASH_NAMESPACE(light_t) light);

EGIHASH_NAMESPACE(full_t) EGIHASH_NAMESPACE(full_new)(EGIHASH_NAMESPACE(light_t) light, EGIHASH_NAMESPACE(callback) callback);
uint64_t EGIHASH_NAMESPACE(full_dag_size)(EGIHASH_NAMESPACE(full_t) full);
void const * EGIHASH_NAMESPACE(full_dag)(EGIHASH_NAMESPACE(full_t) full);
EGIHASH_NAMESPACE(result_t) EGIHASH_NAMESPACE(full_compute)(EGIHASH_NAMESPACE(full_t) full, EGIHASH_NAMESPACE(h256_t) header_hash, uint64_t nonce);
void EGIHASH_NAMESPACE(full_delete)(EGIHASH_NAMESPACE(full_t) full);
#endif

void egihash_h256_compute(EGIHASH_NAMESPACE(h256_t) * output_hash, void * input_data, uint64_t input_size);
void egihash_h512_compute(EGIHASH_NAMESPACE(h512_t) * output_hash, void * input_data, uint64_t input_size);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
