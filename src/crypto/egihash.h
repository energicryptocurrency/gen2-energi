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
#include <cstring>

namespace egihash
{
	bool test_function() noexcept;

	namespace constants
	{
		/** \brief DAG_MAGIC_BYTES is the starting sequence of a DAG file, used for identification.
		*/
		static constexpr char DAG_MAGIC_BYTES[] = "NRGHASH_DAG";

		/** \brief DAG_FILE_HEADER_SIZE is the expected size of a DAG file header.
		*/
		static constexpr uint32_t DAG_FILE_HEADER_SIZE = 64u;

		/** \brief DAG_FILE_MINIMUM_SIZE is the size of the DAG file at epoch 0.
		*/
		static constexpr uint64_t DAG_FILE_MINIMUM_SIZE = 2641099136;

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

		/** \brief The growth of the dataset in bytes per epoch.
		*/
		static constexpr uint32_t DATASET_BYTES_GROWTH = 1u << 23u;

		/** \brief The number of bytes in the dataset at genesis.
		*/
		static constexpr uint32_t DATASET_BYTES_INIT = (1u << 30u) + (DATASET_BYTES_GROWTH * 182);

		/** \brief The growth of the cache in bytes per epoch.
		*/
		static constexpr uint32_t CACHE_BYTES_GROWTH = 1u << 17u;

		/** \brief The number of bytes in the cache at genesis.
		*/
		static constexpr uint32_t CACHE_BYTES_INIT = (1u << 24u) + (CACHE_BYTES_GROWTH * 182);

		/** \brief Ratio of the size of the DAG in bytes relative to the size of the cache in bytes..
		*/
		static constexpr uint32_t CACHE_MULTIPLIER=1024u;

		/** \brief The number of blocks which constitute one epoch.
		*
		*	The DAG and cache must be regenerated once per epoch. (approximately 120 hours)
		*/
		static constexpr uint32_t EPOCH_LENGTH = 7200u;

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
	}

	/** \brief node union is used instead of the native integer to allow both bytes level access and as a 4 byte hash word
	*
	*/
	#pragma pack(push, 1)
	union node
	{
		uint32_t hword;
		uint8_t  bytes[4];
		node()
		:hword{}
		{}

		node(uint8_t bytes_[4])
		{
			::memcpy(bytes, bytes_, 4);
		}

		node(uint32_t hword_):hword(hword_)
		{}
	};
	#pragma pack(pop)
	static_assert(sizeof(node) == sizeof(uint32_t), "Invalid hash node size");


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
	*		All seed hashes for subsequent epochs will be generated from this seedhash.
	*		This hash was chosen as: seed = SHA256(SHA256(Concatenate(EthereumBlock5439314Hash, DashBlock853406Hash)))
	* 		Using two block hashes from other blockchains serves as a proof of the timestamp in the genesis block.
	*
	*	This represents a keccak-256 hash that will be used as input for building the DAG/cache.
	*/
	static constexpr char epoch0_seedhash[] = "\xe8\xbc\xb1\xcf\x8a\x60\x16\x25\x11\x7e\x59\xb5\xf2\xdc\x8c\x36\x6e\x14\x04\x83\x0a\xe9\xd2\x5f\x65\x2b\xe6\x7a\xc9\xbb\x81\x5b";
	static constexpr uint8_t size_epoch0_seedhash = sizeof(epoch0_seedhash) - 1;
	static_assert(size_epoch0_seedhash == 32, "Invalid seedhash");

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

		/** \brief Get the hex-encoded value for this h256_t.
		*
		*	\returns std::string containing hex encoded value for this h256_t.
		*/
		::std::string to_hex() const;

		/** \brief Test if this hash is valid. Returns true if hash data is not all 0 bytes.
		*/
		operator bool() const;

		/** \brief Compare this h256_t to another h256_t.
		*
		*	\return true if the hashes are equal.
		*/
		bool operator==(h256_t const &) const;

		/** \brief This member stores the 256-bit hash data
		*/
		uint8_t b[hash_size];
	};

	/** \brief A default constructed h256_t has all empty bytes.
	*/
	static constexpr h256_t empty_h256;

	/** \brief h512_t represents a the result of a keccak-512 hash.
	*/
	struct h512_t
	{
		using size_type = ::std::size_t;
		static constexpr size_type hash_size = 64;

		/** \brief Default constructor for h512_t fills data field b with 0 bytes
		*/
		constexpr h512_t(): b{0} {}

		/** \brief Default copy constructor
		*/
		h512_t(h512_t const &) = default;

		/** \brief Default copy assignment operator
		*/
		h512_t & operator=(h512_t const &) = default;

		/** \brief Default move constructor
		*/
		h512_t(h512_t &&) = default;

		/** \brief Default move assignment operator
		*/
		h512_t & operator=(h512_t &&) = default;

		/** \brief Default destructor
		*/
		~h512_t() = default;

		/** \brief Construct and compute a hash from a data source.
		*
		*	\param input_data A pointer to the start of the data to be hashed
		*	\param input_size The number of bytes of input data to hash
		*	\throws hash_exception on error
		*/
		h512_t(void const * input_data, size_type input_size);

		/** \brief Get the hex-encoded value for this h512_t.
		*
		*	\returns std::string containing hex encoded value for this h512_t.
		*/
		::std::string to_hex() const;

		/** \brief Test if this hash is valid. Returns true if hash data is not all 0 bytes.
		*/
		operator bool() const;

		/** \brief Compare this h512_t to another h512_t.
		*
		*	\return true if the hashes are equal.
		*/
		bool operator==(h512_t const &) const;

		/** \brief This member stores the 512-bit hash data
		*/
		uint8_t b[hash_size];
	};

	/** \brief A default constructed h512_t has all empty bytes.
	*/
	static constexpr h512_t empty_h512;

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

		/** \brief Compare this result_t to another result_t.
		*
		*	\return true if both the value and mixhash are equal.
		*/
		bool operator==(result_t const &) const;

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
        dag_loading,        /**< dag_loading is loading the DAG from disk */
        dag_generateAndSave /**< dag_generateAndSave is generating and immediately saving the DAG to the disk */
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
		using size_type = uint64_t;

		/** \brief data_type is the underlying data store which stores a cache.
		*/
		using data_type = ::std::vector<::std::vector<node>>;

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

		/** \brief Construct a cache_t given a block number and a progress callback function.
		*
		*	\param block_number is the block number for which this cache_t is to be constructed.
		*	\param callback (optional) may be used to monitor the progress of cache generation. Return false to cancel, true to continue.
		*/
		cache_t(uint64_t block_number, progress_callback_type callback = [](size_type, size_type, int){ return true; });

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

		/** \brief Get the seedhash for this cache.
		*
		*	/returns h256_t seed hash for this cache.
		*/
		h256_t seedhash() const;

		/** \brief Unload cache.
		*
		*	To actually free a cache from memory, call this function on a cache.
		*	The cache will then be released from the internal cache.
		*	Once all references to the cache for this epoch are destroyed, it will be freed.
		*/
		void unload() const;

		/** \brief Get the size of the cache data in bytes.
		*
		*	\param block_number is the block number for which cache size to compute.
		*	\returns size_type representing the size of the cache data in bytes.
		*/
		static size_type get_cache_size(uint64_t const block_number) noexcept;

		/** \brief get_seedhash(uint64_t) will compute the seedhash for a given block number.
		*
		*	\param block_number An unsigned 64-bit integer representing the block number for which to compute the seed hash.
		*	\return An h256_t keccak-256 seed hash for the given block number.
		*/
		static h256_t get_seedhash(uint64_t const block_number);

		/** \brief Determine whether the cache_t for this epoch is already loaded
		*
		*	\param epoch is the epoch number for which to determine if a cache_t is already loaded.
		*	\return bool true if we have this cache_t epoch loaded, false otherwise.
		*/
		static bool is_loaded(uint64_t const epoch);

		/** \brief Get the epoch numbers for which we already have a cache_t loaded.
		*
		*	\return ::std::vector containing epoch numbers for cache_t's that are already loaded.
		*/
		static ::std::vector<uint64_t> get_loaded();

		/** \brief cache_t internal implementation.
		*/
		struct impl_t;

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
		using data_type = ::std::vector<::std::vector<node>>;

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

        /** \brief Generate and Save the DAG to a file fur future loading.
        *
        *	\param block_number is the block number for which the dag will be generated.
        *	\param file_path is the path to the file the DAG should be saved to.
        *	\param callback (optional) may be used to monitor the progress of DAG generation and saving progress.
        *    Return false to cancel, true to continue.
        */
        static void generateAndSave(uint64_t block_number, ::std::string const & file_path, progress_callback_type callback = [](size_type, size_type, int){ return true; });

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

		/** \brief Determine whether the DAG for this epoch is already loaded
		*
		*	\param epoch is the epoch number for which to determine if a DAG is already loaded.
		*	\return bool true if we have this DAG epoch loaded, false otherwise.
		*/
		static bool is_loaded(uint64_t const epoch);

		/** \brief Get the epoch numbers for which we already have a DAG loaded.
		*
		*	\return ::std::vector containing epoch numbers for DAGs that are already loaded.
		*/
		static ::std::vector<uint64_t> get_loaded();

        /** \brief Checks if the DAG file is corrupted or no. It is checking if file exists and dag size is ok
        *
        *	\return returns false if dag file exists and size is ok,
        *   otherwise dag is condiered as corrupted and true is returned
        */
        static bool is_dag_file_corrupted(std::string dag_file);

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

#endif // __cplusplus
