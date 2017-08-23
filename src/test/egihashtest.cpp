#include <iomanip>
#include "egihash/egihash.h"
#include "egihash/internal.h"

#ifdef _WIN32
#include <windows.h>
#include <Shlobj.h>
#endif

//#define BOOST_TEST_MODULE Daggerhashimoto
//#define BOOST_TEST_MAIN

#include <iostream>
#include <functional>
#include <fstream>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;
using byte = uint8_t;
using bytes = std::vector<byte>;
namespace fs = boost::filesystem;


std::string toHex(const uint8_t *streamBytes, const uint64_t size)
{
	std::stringstream sHex;
	for( uint64_t iter = 0; iter < size; ++iter )
	{
		sHex << std::nouppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(streamBytes[iter]);
	}
	return sHex.str();
}

template <typename HashType
, size_t HashSize
, void (*Compute)(HashType * output_hash, void * input_data, uint64_t input_size)>
struct HashTrait
{
	static constexpr size_t Size = HashSize;
	static constexpr decltype(Compute) compute = Compute;
	using Type = HashType;
};


template<typename HashTrait>
void test_hash_func()
{
	string filename = string("hashcache_") + std::to_string(HashTrait::Size) + ".csv";
	    fs::path hcPath = fs::current_path() / "data" / filename;
	#ifdef TEST_DATA_DIR
	    if (!fs::exists(hcPath))
	    {
	    	hcPath = fs::path(BOOST_PP_STRINGIZE(TEST_DATA_DIR)) / filename;
	    }
	#endif
	    cout << hcPath.string() << endl;
		ifstream hif(hcPath.string().c_str());
		BOOST_REQUIRE_MESSAGE(hif.is_open(), "hash cache missing?");
		if ( hif.is_open() )
		{
			char buffer[1024] = {0};
			while(hif.getline(buffer, sizeof(buffer)))
			{
				string line = buffer;
				auto index = line.find_first_of(',');
				auto hashSource = line.substr(0, index), hashExpected = line.substr(index + 1);
				if ( hashSource.size() == HashTrait::Size / 8 && hashExpected.size() == HashTrait::Size / 4 )
				{
					typename HashTrait::Type input;
					typename HashTrait::Type hashRaw;
					memcpy(&input, hashSource.c_str(), HashTrait::Size / 8);
					HashTrait::compute(&hashRaw, input.b, HashTrait::Size / 8);
					auto actual = toHex((uint8_t*)&hashRaw, HashTrait::Size / 8);
					//cout << hashSource << "," << actual << endl;
					BOOST_REQUIRE_MESSAGE(hashExpected == actual, "\nsource: " << hashSource << "\nexpected: " << hashExpected.c_str() << "\n" << "actual: " << actual.c_str() << "\n");
				}
			}
		}
}

using HashTrait256 = HashTrait<egihash_h256_t, 256, egihash_h256_compute>;
using HashTrait512 = HashTrait<egihash_h512_t, 512, egihash_h512_compute>;


BOOST_AUTO_TEST_CASE(SHA3256) {
	test_hash_func<HashTrait256>();
}

BOOST_AUTO_TEST_CASE(SHA3512) {
	test_hash_func<HashTrait512>();
}

