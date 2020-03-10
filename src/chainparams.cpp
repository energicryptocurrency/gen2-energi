// Copyright (c) 2017-2019 The Energi Core developers
// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include "arith_uint256.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"
#include "arith_uint256.h"
#include "hdchain.h"
#include "versionbits.h"

int CChainParams::ExtCoinType(int version) const {
    switch(version) {
    case HDVersion::LEGACY:
        return nLegacyExtCoinType;
    case HDVersion::CURRENT:
    default:
        return nExtCoinType;
    }
}


static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint64_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << nBits << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nHeight  = 0;
    genesis.hashMix.SetNull();
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static CBlock CreateDevNetGenesisBlock(const uint256 &prevBlockHash, const std::string& devNetName, uint32_t nTime, uint32_t nNonce, uint32_t nBits, const CAmount& genesisReward)
{
    assert(!devNetName.empty());

    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    // put height (BIP34) and devnet name into coinbase
    txNew.vin[0].scriptSig = CScript() << 1 << std::vector<unsigned char>(devNetName.begin(), devNetName.end());
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = CScript() << OP_RETURN;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nHeight  = 0;
    genesis.hashMix.SetNull();
    genesis.nNonce   = nNonce;
    genesis.nVersion = 4;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock = prevBlockHash;
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=00000ffd590b14, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=e0028e, nTime=1390095618, nBits=1e0ffff0, nNonce=28917698, vtx=1)
 *   CTransaction(hash=e0028e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d01044c5957697265642030392f4a616e2f3230313420546865204772616e64204578706572696d656e7420476f6573204c6976653a204f76657273746f636b2e636f6d204973204e6f7720416363657074696e6720426974636f696e73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0xA9037BAC7050C479B121CF)
 *   vMerkleTree: e0028e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint64_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "World Power";
    const CScript genesisOutputScript = CScript() << ParseHex("0479619b3615fc9f03aace413b9064dc97d4b6f892ad541e5a2d8a3181517443840a79517fb1a308e834ac3c53da86de69a9bcce27ae01cf77d9b2b9d7588d122a") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

bool GenesisCheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return error("GenesisCheckProofOfWork(): nBits below minimum work");

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return error("GenesisCheckProofOfWork(): hash doesn't match nBits");

    return true;
}


//#define ENERGI_MINE_NEW_GENESIS_BLOCK
#ifdef ENERGI_MINE_NEW_GENESIS_BLOCK

#include "dag_singleton.h"
#include "crypto/egihash.h"
#include "validation.h"

#include <chrono>
#include <iomanip>

struct GenesisMiner
{
    GenesisMiner(CBlock & genesisBlock, std::string networkID)
    {
        using namespace std;

        arith_uint256 bnTarget = arith_uint256().SetCompact(genesisBlock.nBits);
        prepare_dag();

        auto start = std::chrono::system_clock::now();

        genesisBlock.nTime = chrono::seconds(time(NULL)).count();
        int i = 0;
        while (true)
        {
            uint256 powHash = genesisBlock.GetPOWHash();

            if ((++i % 250000) == 0)
            {
                auto end = chrono::system_clock::now();
                auto elapsed = chrono::duration_cast<std::chrono::milliseconds>(end - start);
                cout << i << " hashes in " << elapsed.count() / 1000.0 << " seconds ("
                    << static_cast<double>(i) / static_cast<double>(elapsed.count() / 1000.0) << " hps)" << endl;
            }

            if (UintToArith256(powHash) < bnTarget)
            {
                auto end = chrono::system_clock::now();
                auto elapsed = chrono::duration_cast<std::chrono::milliseconds>(end - start);
                cout << "Mined genesis block for " << networkID << " network: 0x" << genesisBlock.GetHash().ToString() << endl
                    << "target was " << bnTarget.ToString() << " POWHash was 0x" << genesisBlock.GetPOWHash().ToString() << endl
                    << "took " << i << " hashes in " << elapsed.count() / 1000.0 << " seconds ("
                    << static_cast<double>(i) / static_cast<double>(elapsed.count() / 1000.0) << " hps)" << endl << endl
                    << genesisBlock.ToString() << endl;
                exit(0);
            }
            genesisBlock.nNonce++;
        }
    }

    void prepare_dag()
    {
        using namespace egihash;
        using namespace std;
        auto const & seedhash = cache_t::get_seedhash(0).to_hex();

        stringstream ss;
        ss << hex << setw(4) << setfill('0') << 0 << "-" << seedhash.substr(0, 12) << ".dag";
        auto const epoch_file = GetDataDir(false) / "dag" / ss.str();

        auto progress = [](::std::size_t step, ::std::size_t max, int phase) -> bool
        {
            switch(phase)
            {
                case cache_seeding:
                    cout << "\rSeeding cache...";
                    break;
                case cache_generation:
                    cout << "\rGenerating cache...";
                    break;
                case cache_saving:
                    cout << "\rSaving cache...";
                    break;
                case cache_loading:
                    cout << "\rLoading cache...";
                    break;
                case dag_generation:
                    cout << "\rGenerating DAG...";
                    break;
                case dag_saving:
                    cout << "\rSaving DAG...";
                    break;
                case dag_loading:
                    cout << "\rLoading DAG...";
                    break;
                default:
                    break;
            }
            cout << fixed << setprecision(2)
            << static_cast<double>(step) / static_cast<double>(max) * 100.0 << "%"
            << setfill(' ') << setw(80) << flush;

            return true;
        };
        unique_ptr<dag_t> new_dag(new dag_t(epoch_file.string(), progress));
        cout << "\r" << endl << endl;
        ActiveDAG(move(new_dag));
    }
};
#endif // ENERGI_MINE_NEW_GENESIS_BLOCK

static CBlock FindDevNetGenesisBlock(const Consensus::Params& params, const CBlock &prevBlock, const CAmount& reward)
{
    std::string devNetName = GetDevNetName();
    assert(!devNetName.empty());

    CBlock block = CreateDevNetGenesisBlock(prevBlock.GetHash(), devNetName.c_str(), prevBlock.nTime + 1, 0, prevBlock.nBits, reward);

    arith_uint256 bnTarget;
    bnTarget.SetCompact(block.nBits);

    for (uint32_t nNonce = 0; nNonce < UINT32_MAX; nNonce++) {
        block.nNonce = nNonce;

        uint256 hash = block.GetPOWHash();
        if (UintToArith256(hash) <= bnTarget)
            return block;
    }

    // This is very unlikely to happen as we start the devnet with a very low difficulty. In many cases even the first
    // iteration of the above loop will give a result already
    error("FindDevNetGenesisBlock: could not find devnet genesis block for %s", devNetName);
    assert(false);

    return block;
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */


class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";

        // Energi distribution parameters
        consensus.energiBackboneScript = CScript() << OP_HASH160 << ParseHex("b051bdceb44b28bb36ef2add5ec07ccbc64708c2") << OP_EQUAL;

        // Seeing as there are 526,000 blocks per year, and there is a 12M annual emission
        // masternodes get 40% of all coins or 4.8M / 526,000 ~ 9.14
        // miners get 10% of all coins or 1.2M / 526,000 ~ 2.28
        // backbone gets 10% of all coins or 1.2M / 526,000 ~ 2.28
        // which adds up to 13.7 as block subsidy
        consensus.nBlockSubsidy = 1370000000ULL;
        // 10% to energi backbone
        consensus.nBlockSubsidyBackbone = 228000000ULL;
        // 10% miners
        consensus.nBlockSubsidyMiners = 228000000ULL;
        // 40% masternodes
        // each masternode is paid serially.. more the master nodes more is the wait for the payment
        // masternode payment gap is masternodes minutes
        consensus.nBlockSubsidyMasternodes = 914000000ULL;

        // ensure the sum of the block subsidy parts equals the whole block subsidy
        assert(consensus.nBlockSubsidyBackbone + consensus.nBlockSubsidyMiners + consensus.nBlockSubsidyMasternodes == consensus.nBlockSubsidy);

        // 40% of the total annual emission of ~12M goes to the treasury
        // which is around 4.8M / 26.07 ~ 184,000, where 26.07 are the
        // number of super blocks per year according to the 20160 block cycle
        consensus.nSuperblockCycle = 20160; // (60*24*14) Super block cycle for every 14 days (2 weeks)
        consensus.nRegularTreasuryBudget = 18400000000000ULL;
        consensus.nSpecialTreasuryBudget = 400000000000000ULL + consensus.nRegularTreasuryBudget; // 4 million extra coins for the special budget cycle
        consensus.nSpecialTreasuryBudgetBlock = consensus.nSuperblockCycle * 2;

        consensus.nMasternodePaymentsStartBlock = 216000; // should be about 150 days after genesis
        consensus.nInstantSendKeepLock = 24;
        consensus.nInstantSendConfirmationsRequired = 6;

        consensus.nGovernanceMinQuorum = 7;
        consensus.nGovernanceFilterElements = 20000;

        consensus.nMasternodeMinimumConfirmations = 15;
        consensus.BIP34Height = 1; // since genesis
        consensus.BIP65Height = 1; // since genesis
        consensus.BIP66Height = 1; // since genesis
        consensus.DIP0001Height = 1; // since genesis
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 20
        consensus.posLimit = uint256S("000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 24

        consensus.nPowTargetTimespan = 24 * 60 * 60; // Energi: 1 day
        consensus.nPowTargetSpacing = 60; // Energi: 1 minute
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = VERSIONBITS_NUM_BITS - 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1486252800; // Feb 5th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1556013600; // Apr 23th, 2019

        // Deployment of DIP0001
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1508025600; // Oct 15th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1556013600; // Apr 23th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 4032;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 3226; // 80% of 4032

        // Deployment of BIP147
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1546300800; // Jan 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1556013600; // Apr 23th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 4032;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 3226; // 80% of 4032

        // Deployment of SPORK17
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].nStartTime = 1566129600; // Aug 18th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].nTimeout = 1577793600; // Dec 31st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].nThreshold = 75; // 50% of 100

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000068567ac396c6ab768");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x6a558d604e0760e4b6990a70a83786bfbb28c57ea9569984bb4186c5a24b6278");

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xec;
        pchMessageStart[1] = 0x2d;
        pchMessageStart[2] = 0x9a;
        pchMessageStart[3] = 0xaf;
        vAlertPubKey = ParseHex("048cd9adbefe1ca8435de5372e2725027e56f959fb979f5252c7d2a51de2f5251c10d55ad632e8c217d086b7b517ccfa934d5af693f354a0ab58bce23c963df5fc");
        nDefaultPort = 9797;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1523716938, 34766776, 0x1e0ffff0, 1, consensus.nBlockSubsidyBackbone + consensus.nBlockSubsidyMiners);
        bool const valid_genesis_pow = GenesisCheckProofOfWork(genesis.GetPOWHash(), genesis.nBits, consensus);
        consensus.hashGenesisBlock = genesis.GetHash();

        uint256 expectedGenesisHash = uint256S("0x8b5f13fa7ebd7d8b6280c2df0e6f5b16e7c510b20dc5c3411151f65a0c020e31");
        uint256 expectedGenesisMerkleRoot = uint256S("0xce737517317ef573bb17f34c49e10fa30357983f29821f129a99fe3cb90e34c4");

        #ifdef ENERGI_MINE_NEW_GENESIS_BLOCK
        if (consensus.hashGenesisBlock != expectedGenesisHash)
        {
            GenesisMiner mine(genesis, strNetworkID);
        }
        #endif // ENERGI_MINE_NEW_GENESIS_BLOCK

        assert(valid_genesis_pow);
        assert(consensus.hashGenesisBlock == expectedGenesisHash);
        assert(genesis.hashMerkleRoot == expectedGenesisMerkleRoot);

        vSeeds.push_back(CDNSSeedData("energi.network", "dnsseed.energi.network"));

        // Energi addresses start with 'E'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,33);
        // Energi script addresses start with 'N'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,53);
        // Energi private keys start with 'G'
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,106);
        // Energi BIP32 pubkeys start with 'npub'
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x03)(0xB8)(0xC8)(0x56).convert_to_container<std::vector<unsigned char> >();
        // Energi BIP32 prvkeys start with 'nprv'
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0xD7)(0xDC)(0x6E)(0x9F).convert_to_container<std::vector<unsigned char> >();

        // Energi BIP44/SLIP44 coin type is '9797'
        nExtCoinType = 9797;
        // Legacy inherited from Dash
        nLegacyExtCoinType = 5;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fAllowMultipleAddressesFromGroup = false;
        fAllowMultiplePorts = false;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour

        // See https://github.com/dashpay/dash/pull/1969
        // Energi prefix: Base58 'E' = 33 = 0x21
        strSporkAddress = "EWk4DawoXbRqGfcyQ2bM2DWVA11TejDz8u";

        nStakeMinAge = 3600;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of

            (  10000, uint256S("0x9e7d44bb9b9d8e0ad655477c7fd753d11df321a889835c9b940a2342d8e43f3c"))
            (  20000, uint256S("0x56e3033c6e8d56c073d5cd0b7ea59f70ec075fa6660054e2a67bfcdc853d8cb9"))
            (  30000, uint256S("0x90f9c4d79134f8e8aa8a64181ebebeadc4914d982dc56d81b40b0d9c04e14132"))
            (  32000, uint256S("0x423d1fcaef88449d94a74b803055f165b1b2c677e10487e63cf3a55b53cff82a"))
            (  50000, uint256S("0x0823370fb037369fcbf28e0de607733bbad1133343c58674a48654dfa690b15c"))
            (  75000, uint256S("0x50b98feae42b5acdd36c5f75582e4eb9780a5ae0901992985b9aaf58ba6e4e71"))
            (  94800, uint256S("0x8cddf9609d578281ed60a79900522f45c4eab1406f336d2109c61619c370828f"))
            ( 100000, uint256S("0x165dd668bb60e90752161dc777a89946d224769ed3b1a10ccfca5693db999b03"))
            ( 150000, uint256S("0x47947121d1025b7881fd7583df130f2bbb60a36fa73fe2ca41fe30419a4d0aab"))
            ( 200000, uint256S("0xbf363e6e74c2bf2f1eba5ee4afb85f48d29fc44ceb8260e847b76dfa70c8868b"))
            ( 250000, uint256S("0x835d278a931b3b39ed42ebdad1402fff51c1de05cbd3660a8877c989f1e3d202"))
            ( 281000, uint256S("0x64e17d3d00946e8fd82ef4991ca864d63b7274a8c89809c9bef9a3c60e713f95"))
            ( 310000, uint256S("0xca5cf4e91d19f5258326f677f4c53ac87d799bd7208a8cc9183438f8a7baa573"))
            ( 343000, uint256S("0x8018bb616584d31bb568a5867baf06742a99bdb1b55c541364a0587f07779036"))
            ( 360000, uint256S("0x7bfb0cb837ab87389241dc9bb4c8881fde6ec7bdfe5f39d41bf6c886733cc8cb"))
            ( 370000, uint256S("0x461c0314744dc87237719076f281b7be1e724b77563d656267317cd727f781c7"))
            ( 383040, uint256S("0x9d662c1e343b626f43b431ffdda02378b47d4dc34967d73102330650a493ad3c"))
            ( 383300, uint256S("0xbc4db5c524f4ebda508874f2ddc039e5492345a3b4330168eb67acca8c8b791b"))
            ( 400000, uint256S("0x2d70c456952952df17b5fd1bb0dec6c6a91fb999c617941ea5585ebd91e0b89d"))
            // End of PoW era
            ( 437600, uint256S("0x47fafe6acbb9ef96668f7af235d5f2b36cbe0c0a865a4ea07748c2fc1a2afaf5"))
            ( 437650, uint256S("0x3b2f28e4c3b32b6107a672462e3a0fe29a83e7fe92423e1f36555728bf0b7bf2"))
            ( 437750, uint256S("0x1de7558fddf1a26f38ddab2691bbd470bd3b341aed5065d90b569c7b98c3be0d"))
            ( 440000, uint256S("0x0099fdea7d3b184ae7a4a10b114e384d76eeca520d95bdbe6cc1934c0bc6bf10"))
            ( 500000, uint256S("0xfa92bd32635288eca40363395d1bba4858c435d932d6d7ef717dd8095f9dbf3d"))
            ( 585000, uint256S("0xdac8c2ec27a7024fe4a0fbc3c11c0b2f509e6689fa19e59a1a895af716ba4696"))
            ( 750000, uint256S("0xffd36d48b85d8486ce2dae20e9e35df31d26a7706e798e7c65b61c226a239aa0"))
            (1108550, uint256S("0x6a558d604e0760e4b6990a70a83786bfbb28c57ea9569984bb4186c5a24b6278"))
            (1108551, uint256S("0x00000000000000000000000000000000ffffffffffffffffffffffffffffffff"))

            // Blacklist
            ,
            {}
     };

        chainTxData = ChainTxData{
            1583850974,     // * UNIX timestamp of last known number of transactions
            3877096,        // * total number of transactions between genesis and that timestamp
                            //   (the tx=... number in the SetBestChain debug.log lines)
            0.2356          // * estimated number of transactions per second after that timestamp
        };
    }
};
static CMainParams mainParams;

/**
 * Testnet (v1)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";

        // Energi distribution parameters
        consensus.energiBackboneScript = CScript() << OP_DUP << OP_HASH160 << ParseHex("b506a5b17506bab7a7e68ee557046d64a01a6f0d") << OP_EQUALVERIFY << OP_CHECKSIG;

        // Seeing as there are 526,000 blocks per year, and there is a 12M annual emission
        // masternodes get 40% of all coins or 4.8M / 526,000 ~ 9.14
        // miners get 10% of all coins or 1.2M / 526,000 ~ 2.28
        // backbone gets 10% of all coins or 1.2M / 526,000 ~ 2.28
        // which adds up to 13.7 as block subsidy
        consensus.nBlockSubsidy = 1370000000ULL;
        // 10% to energi backbone
        consensus.nBlockSubsidyBackbone = 228000000ULL;
        // 10% miners
        consensus.nBlockSubsidyMiners = 228000000ULL;
        // 40% masternodes
        // each masternode is paid serially.. more the master nodes more is the wait for the payment
        // masternode payment gap is masternodes minutes
        consensus.nBlockSubsidyMasternodes = 914000000ULL;

        // ensure the sum of the block subsidy parts equals the whole block subsidy
        assert(consensus.nBlockSubsidyBackbone + consensus.nBlockSubsidyMiners + consensus.nBlockSubsidyMasternodes == consensus.nBlockSubsidy);

        // 40% of the total annual emission of ~12M goes to the treasury
        // which is around 4.8M / 26.07 ~ 184,000, where 26.07 are the
        // number of super blocks per year according to the 180 block cycle
        consensus.nSuperblockCycle = 180;
        consensus.nRegularTreasuryBudget = 18400000000000ULL;
        consensus.nSpecialTreasuryBudget = 400000000000000ULL + consensus.nRegularTreasuryBudget; // 4 million extra coins for the special budget cycle
        consensus.nSpecialTreasuryBudgetBlock = consensus.nSuperblockCycle * 50;

        consensus.nMasternodePaymentsStartBlock = 17630; // should be about 15 days after genesis
        consensus.nInstantSendKeepLock = 6;
        consensus.nInstantSendConfirmationsRequired = 2;

        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimit = uint256S("0fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 4
        consensus.nPowTargetTimespan = 24 * 60 * 60; // in seconds -> Energi: 1 day
        consensus.nPowTargetSpacing = 60; // in seconds Energi: 1 minute
        consensus.BIP34Height = 1; //
        consensus.BIP65Height = 1; //
        consensus.BIP66Height = 1; //
        consensus.DIP0001Height = 1; //
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = VERSIONBITS_NUM_BITS - 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1486252800; // Feb 5th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1549328400; // Feb 5th, 2019

        // Deployment of DIP0001
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1505692800; // Sep 18th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1549328400; // Feb 5th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 50; // 50% of 100

        // Deployment of BIP147
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1546300800; // Jan 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1549328400; // Feb 5th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 50; // 50% of 100

        // Deployment of SPORK17
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].nStartTime = 1566129600; // Aug 18th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].nTimeout = 1577793600; // Dec 31st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].nThreshold = 75; // 50% of 100

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000045a497bb59ef8e26");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x9b5e54cd8b8634127bee5f57038a77d887947a11934427fea28ae38d74502508");

        pchMessageStart[0] = 0xd9;
        pchMessageStart[1] = 0x2a;
        pchMessageStart[2] = 0xab;
        pchMessageStart[3] = 0x6e;
        vAlertPubKey = ParseHex("04da7109a0215bf7bb19ecaf9e4295104142b4e03579473c1083ad44e8195a13394a8a7e51ca223fdbc5439420fd08963e491007beab68ac65c5b1c842c8635b37");
        nDefaultPort = 19797;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1524344801, 16880322, 0x207fffff, 1, consensus.nBlockSubsidyBackbone + consensus.nBlockSubsidyMiners);
        bool const valid_genesis_pow = GenesisCheckProofOfWork(genesis.GetPOWHash(), genesis.nBits, consensus);
        consensus.hashGenesisBlock = genesis.GetHash();

        uint256 expectedGenesisHash = uint256S("0xee84bfa5f6cafe2ba7f164cee0c33ec63aca76edffa4e8e94656a9be2262cf74");
        uint256 expectedGenesisMerkleRoot = uint256S("0x34e077f3b96691e4f1aea04061ead361fc4f5b45250513199f46f352b7e4669e");

        // TODO: mine genesis block for testnet
        #ifdef ENERGI_MINE_NEW_GENESIS_BLOCK
        if (consensus.hashGenesisBlock != expectedGenesisHash)
        {
            GenesisMiner mine(genesis, strNetworkID);
        }
        #endif // ENERGI_MINE_NEW_GENESIS_BLOCK

        assert(valid_genesis_pow);
        assert(consensus.hashGenesisBlock == expectedGenesisHash);
        assert(genesis.hashMerkleRoot == expectedGenesisMerkleRoot);

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("test.energi.network", "dnsseed.test.energi.network"));

        // Testnet Energi addresses start with 't'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,127);
        // Testnet Energi script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Testnet Energi BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        // Testnet Energi BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Dedicated NRG testnet for BIP44
        nExtCoinType = 19797;
        // BIP44 test coin type is '1' (All coin's testnet default)
        nLegacyExtCoinType = 1;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fAllowMultipleAddressesFromGroup = false;
        fAllowMultiplePorts = false;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        // See https://github.com/dashpay/dash/pull/1969
        // Energi prefix: Base58 't' = 127 = 0x7F
        strSporkAddress = "tCLzFoAUkWyrDJmU3qvcKpSA41aD6AckwL";

        nStakeMinAge = 180;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (  1000, uint256S("0x48357913ab6aeff3ac5d8a7120cdf991ca7b598f40c30efbc66b32ce343c8596"))
            (  5000, uint256S("0x50d6318ae28e2d46d3aa5ecb4a7566ec3e9f8b9542e9a84a744d3c8eb815f405"))
            (  9000, uint256S("0x263bb5d663abbbff11318d82c93249c63523f6b48535f81acf194e45e353be59"))
            (  17606, uint256S("0xa3a707f57db9100fcc949609394c2ab164e51892f1b3810bbedad8f9cfb87f91"))
            (  20000, uint256S("0x8085fb36bd44f4e238b10f948f9c944f2185f93164f567e43f477097dad59dda"))
            (  25000, uint256S("0x59049b920a2cbf4f61fce77ef3341ff9a393e0b70b734cc5812902dd105f4de8"))
            (  31000, uint256S("0xf2b400d4d516ac50cfb806d365e26e55c4e216e21f678f3e60405942bf266409"))
            (  45000, uint256S("0xb4f8601acbca2073fde7691c58145886534ae4c6806ccdf3bfff77d3fa6acbaf"))
            (  47000, uint256S("0x83c326375f94cfa64dfd9f6250259f3bff8e744d8f8a0c53546abe205f6faa45"))
            (  47170, uint256S("0xb7ac4454f458096ab3510e408d6c8c7cb2214ecbda019a3019482922f8c083e2"))
            (  57000, uint256S("0x9b5e54cd8b8634127bee5f57038a77d887947a11934427fea28ae38d74502508"))
            (  74741, uint256S("0xead94a1aa85edc4318c0e1947dd88fa64935c86be540c5810023d8f0b86dadac"))
            (  86000, uint256S("0x75951ec8094fc01ab2be643eb48cc10638690470b8550d7dd8bf849eaafff7bc"))
            ( 285000, uint256S("0x41b6a89e456562d1b9ad17532bfb724491b5fea0a8fc255cf8e585adecc8de3f"))
            ( 463500, uint256S("0x5993b69fc21046198cb6b43ec1c696a3dcb3213bc60e80ac09a52aa1626afb8f"))
            ( 474775, uint256S("0xce8371e002071f3462f96579b2280afea8dd2f108f20b4f6da5880ef461aa1ac"))

            // Blacklist
            ,
            {}
        };

        chainTxData = ChainTxData{
            1566647180,     // * UNIX timestamp of last checkpoint block
            906917,         // * total number of transactions between genesis and last checkpoint
                            //   (the tx=... number in the SetBestChain debug.log lines)
            0.0461          // * estimated number of transactions per second after that timestamp
        };

    }
};
static CTestNetParams testNetParams;

#ifdef ENERGI_ENABLE_TESTNET_60X
/**
 * Testnet (60x)
 */
class CTestNet60xParams : public CChainParams {
public:
    CTestNet60xParams() {
        strNetworkID = "test60";

        // Energi distribution parameters
        consensus.energiBackboneScript = CScript() << OP_DUP << OP_HASH160 << ParseHex("b506a5b17506bab7a7e68ee557046d64a01a6f0d") << OP_EQUALVERIFY << OP_CHECKSIG;

        // Seeing as there are 526,000 blocks per year, and there is a 12M annual emission
        // masternodes get 40% of all coins or 4.8M / 526,000 ~ 9.14
        // miners get 10% of all coins or 1.2M / 526,000 ~ 2.28
        // backbone gets 10% of all coins or 1.2M / 526,000 ~ 2.28
        // which adds up to 13.7 as block subsidy
        consensus.nBlockSubsidy = 1370000000ULL * 60ULL;
        // 10% to energi backbone
        consensus.nBlockSubsidyBackbone = 228000000ULL * 60ULL;
        // 10% miners
        consensus.nBlockSubsidyMiners = 228000000ULL * 60ULL;
        // 40% masternodes
        // each masternode is paid serially.. more the master nodes more is the wait for the payment
        // masternode payment gap is masternodes minutes
        consensus.nBlockSubsidyMasternodes = 914000000ULL * 60ULL;

        // ensure the sum of the block subsidy parts equals the whole block subsidy
        assert(consensus.nBlockSubsidyBackbone + consensus.nBlockSubsidyMiners + consensus.nBlockSubsidyMasternodes == consensus.nBlockSubsidy);

        // 40% of the total annual emission of ~12M goes to the treasury
        // which is around 4.8M / 26.07 ~ 184,000, where 26.07 are the
        // number of super blocks per year according to the 20160 block cycle
        consensus.nSuperblockCycle = 60;
        consensus.nRegularTreasuryBudget = 18400000000000ULL * 60ULL;
        consensus.nSpecialTreasuryBudget = (400000000000000ULL + consensus.nRegularTreasuryBudget) * 60ULL; // 4 million extra coins for the special budget cycle
        consensus.nSpecialTreasuryBudgetBlock = consensus.nSuperblockCycle * 50;

        consensus.nMasternodePaymentsStartBlock = 216000 / 60;
        consensus.nInstantSendKeepLock = 6;

        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 20
        consensus.posLimit = uint256S("000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 24
        consensus.nPowTargetTimespan = 24 * 60 * 60; // in seconds -> Energi: 1 day
        consensus.nPowTargetSpacing = 60; // in seconds Energi: 1 minute
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = VERSIONBITS_NUM_BITS - 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1486252800; // Feb 5th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1517788800; // Feb 5th, 2018

        pchMessageStart[0] = 0xd9;
        pchMessageStart[1] = 0x2a;
        pchMessageStart[2] = 0xab;
        pchMessageStart[3] = 0x60; // Changed the last byte just in case, even though the port is different too, so shouldn't mess with the general testnet
        vAlertPubKey = ParseHex("04da7109a0215bf7bb19ecaf9e4295104142b4e03579473c1083ad44e8195a13394a8a7e51ca223fdbc5439420fd08963e491007beab68ac65c5b1c842c8635b37");
        nDefaultPort = 29797;
        nMaxTipAge = 0x7fffffff; // allow mining on top of old blocks for testnet
        nDelayGetHeadersTime = 24 * 60 * 60;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1523717174, 48131894, 0x1e0ffff0, 1, consensus.nBlockSubsidyBackbone + consensus.nBlockSubsidyMiners);
        bool const valid_genesis_pow = GenesisCheckProofOfWork(genesis.GetPOWHash(), genesis.nBits, consensus);
        consensus.hashGenesisBlock = genesis.GetHash();
        uint256 expectedGenesisHash = uint256S("0x22ede2ac8fd04bdc2adfd06e7b0a3a0cb3aba213d99c36ceeb4a8e031674b64c");
        uint256 expectedGenesisMerkleRoot = uint256S("0x1ee1b1a8bfb343ed27c4a5974a552adf1c22da7551a3a4f595aeb888b31b5a05");

        // TODO: mine genesis block for testnet60x
        #ifdef ENERGI_MINE_NEW_GENESIS_BLOCK
        if (consensus.hashGenesisBlock != expectedGenesisHash)
        {
            GenesisMiner mine(genesis, strNetworkID);
        }
        #endif // ENERGI_MINE_NEW_GENESIS_BLOCK

        assert(valid_genesis_pow);
        assert(consensus.hashGenesisBlock == expectedGenesisHash);
        assert(genesis.hashMerkleRoot == expectedGenesisMerkleRoot);

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("test60x.energi.network",  "dnsseed.test60x.energi.network"));

        // Testnet Energi addresses start with 't'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,127);
        // Testnet Energi script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Testnet Energi BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        // Testnet Energi BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Dedicated NRG testnet for BIP44
        nExtCoinType = 19797;
        // BIP44 test coin type is '1' (All coin's testnet default)
        nLegacyExtCoinType = 1;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test60x, pnSeed6_test60x + ARRAYLEN(pnSeed6_test60x));

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        // See https://github.com/dashpay/dash/pull/1969
        // Energi prefix: Base58 't' = 127 = 0x7F
        strSporkAddress = "tCLzFoAUkWyrDJmU3qvcKpSA41aD6AckwL";

        nStakeMinAge = 180;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 0, uint256S("0x440cbbe939adba25e9e41b976d3daf8fb46b5f6ac0967b0a9ed06a749e7cf1e2")),
            0, // * UNIX timestamp of last checkpoint block
            0,     // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            0         // * estimated number of transactions per day after checkpoint
        };

    }
};
static CTestNet60xParams testNet60xParams;
#endif

/**
 * Devnet
 */
class CDevNetParams : public CChainParams {
public:
    CDevNetParams() {
        strNetworkID = "dev";

        // Energi distribution parameters
        consensus.energiBackboneScript = CScript() << OP_DUP << OP_HASH160 << ParseHex("b506a5b17506bab7a7e68ee557046d64a01a6f0d") << OP_EQUALVERIFY << OP_CHECKSIG;

        // Seeing as there are 526,000 blocks per year, and there is a 12M annual emission
        // masternodes get 40% of all coins or 4.8M / 526,000 ~ 9.14
        // miners get 10% of all coins or 1.2M / 526,000 ~ 2.28
        // backbone gets 10% of all coins or 1.2M / 526,000 ~ 2.28
        // which adds up to 13.7 as block subsidy
        consensus.nBlockSubsidy = 1370000000ULL;
        // 10% to energi backbone
        consensus.nBlockSubsidyBackbone = 228000000ULL;
        // 10% miners
        consensus.nBlockSubsidyMiners = 228000000ULL;
        // 40% masternodes
        // each masternode is paid serially.. more the master nodes more is the wait for the payment
        // masternode payment gap is masternodes minutes
        consensus.nBlockSubsidyMasternodes = 914000000ULL;

        // ensure the sum of the block subsidy parts equals the whole block subsidy
        assert(consensus.nBlockSubsidyBackbone + consensus.nBlockSubsidyMiners + consensus.nBlockSubsidyMasternodes == consensus.nBlockSubsidy);

        // 40% of the total annual emission of ~12M goes to the treasury
        // which is around 4.8M / 26.07 ~ 184,000, where 26.07 are the
        // number of super blocks per year according to the 20160 block cycle
        consensus.nSuperblockCycle = 60;
        consensus.nRegularTreasuryBudget = 18400000000000ULL;
        consensus.nSpecialTreasuryBudget = 400000000000000ULL + consensus.nRegularTreasuryBudget; // 4 million extra coins for the special budget cycle
        consensus.nSpecialTreasuryBudgetBlock = consensus.nSuperblockCycle * 50;

        consensus.nMasternodePaymentsStartBlock = 4010; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nSuperblockCycle = 24; // Superblocks can be issued hourly on devnet
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP34Height = 1; // BIP34 activated immediately on devnet
        consensus.BIP65Height = 1; // BIP65 activated immediately on devnet
        consensus.BIP66Height = 1; // BIP66 activated immediately on devnet
        consensus.DIP0001Height = 2; // DIP0001 activated immediately on devnet
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
        consensus.posLimit = uint256S("0fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 4
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Dash: 1 day
        consensus.nPowTargetSpacing = 2.5 * 60; // Important for PoW test
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = VERSIONBITS_NUM_BITS - 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1506556800; // September 28th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1549328400; // Feb 5th, 2019

        // Deployment of DIP0001
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1505692800; // Sep 18th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1549328400; // Feb 5th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 50; // 50% of 100

        // Deployment of BIP147
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1517792400; // Feb 5th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1549328400; // Feb 5th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 50; // 50% of 100

        // Deployment of SPORK17
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].nStartTime = 1566129600; // Aug 18th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].nTimeout = 1577793600; // Dec 31st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].nThreshold = 75; // 50% of 100

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

        pchMessageStart[0] = 0xe2;
        pchMessageStart[1] = 0xca;
        pchMessageStart[2] = 0xff;
        pchMessageStart[3] = 0xce;
        vAlertPubKey = ParseHex("04517d8a699cb43d3938d7b24faaff7cda448ca4ea267723ba614784de661949bf632d6304316b244646dea079735b9a6fc4af804efb4752075b9fe2245e14e412");
        nDefaultPort = 19797;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1417713337, 1096447, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x6222b3b87293251ef84a00bf7bcafabf3af0bf4631fe2aea3b8fb850e7391806"));
        assert(genesis.hashMerkleRoot == uint256S("0x9339637354dea8affaeccf8df6fd22db5049305d26652be0e503cd468f613ce4"));

        devnetGenesis = FindDevNetGenesisBlock(consensus, genesis, 50 * COIN);
        consensus.hashDevnetGenesisBlock = devnetGenesis.GetHash();

        vFixedSeeds.clear();
        vSeeds.clear();

        // Testnet Energi addresses start with 'y'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
        // Testnet Energi script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Testnet Energi BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        // Testnet Energi BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Testnet Energi BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fAllowMultipleAddressesFromGroup = true;
        fAllowMultiplePorts = true;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        // See for instructions https://github.com/dashpay/dash/pull/1969
        // privKey: cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK
        strSporkAddress = "yj949n1UH6fDhw6HtVE5VMj2iSTaSWBMcW";

        nStakeMinAge = 0;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (      0, uint256S("0x000008ca1832a4baf228eb1553c03d3a2c8e02399550dd6ea8d65cec3ef23d2e"))
            (      1, devnetGenesis.GetHash())

            // Blacklist
            ,
            {}
        };

        chainTxData = ChainTxData{
            devnetGenesis.GetBlockTime(), // * UNIX timestamp of devnet genesis block
            2,                            // * we only have 2 coinbase transactions when a devnet is started up
            0.01                          // * estimated number of transactions per second
        };
    }
};
static CDevNetParams *devNetParams;


/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";

        // Energi distribution parameters
        consensus.energiBackboneScript = CScript() << OP_DUP << OP_HASH160 << ParseHex("b506a5b17506bab7a7e68ee557046d64a01a6f0d") << OP_EQUALVERIFY << OP_CHECKSIG;

        // Seeing as there are 526,000 blocks per year, and there is a 12M annual emission
        // masternodes get 40% of all coins or 4.8M / 526,000 ~ 9.14
        // miners get 10% of all coins or 1.2M / 526,000 ~ 2.28
        // backbone gets 10% of all coins or 1.2M / 526,000 ~ 2.28
        // which adds up to 13.7 as block subsidy
        consensus.nBlockSubsidy = 1370000000ULL;
        // 10% to energi backbone
        consensus.nBlockSubsidyBackbone = 228000000ULL;
        // 10% miners
        consensus.nBlockSubsidyMiners = 228000000ULL;
        // 40% masternodes
        // each masternode is paid serially.. more the master nodes more is the wait for the payment
        // masternode payment gap is masternodes minutes
        consensus.nBlockSubsidyMasternodes = 914000000ULL;

        // ensure the sum of the block subsidy parts equals the whole block subsidy
        assert(consensus.nBlockSubsidyBackbone + consensus.nBlockSubsidyMiners + consensus.nBlockSubsidyMasternodes == consensus.nBlockSubsidy);

        // 40% of the total annual emission of ~12M goes to the treasury
        // which is around 4.8M / 26.07 ~ 184,000, where 26.07 are the
        // number of super blocks per year according to the 20160 block cycle
        consensus.nSuperblockCycle = 60;
        consensus.nRegularTreasuryBudget = 18400000000000ULL;
        consensus.nSpecialTreasuryBudget = 400000000000000ULL + consensus.nRegularTreasuryBudget; // 4 million extra coins for the special budget cycle
        consensus.nSpecialTreasuryBudgetBlock = consensus.nSuperblockCycle * 50;

        consensus.nMasternodePaymentsStartBlock = 240;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 100;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.DIP0001Height = 2000;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
        consensus.posLimit = uint256S("0fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 4
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Energi: 1 day
        consensus.nPowTargetSpacing = 60; // Energi: 1 minute
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = VERSIONBITS_NUM_BITS - 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SPORK17].nTimeout = 999999999999ULL;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xef;
        pchMessageStart[1] = 0x89;
        pchMessageStart[2] = 0x6c;
        pchMessageStart[3] = 0x7f;
        nDefaultPort = 39797;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1524279488, 12, 0x207fffff, 1, consensus.nBlockSubsidyBackbone + consensus.nBlockSubsidyMiners);
        bool const valid_genesis_pow = GenesisCheckProofOfWork(genesis.GetPOWHash(), genesis.nBits, consensus);
        consensus.hashGenesisBlock = genesis.GetHash();

        uint256 expectedGenesisHash = uint256S("0x378abe3d42888769177494063edd42e6c3925e938ff8f73c71a6b6ad5b293ea7");
        uint256 expectedGenesisMerkleRoot = uint256S("0x34e077f3b96691e4f1aea04061ead361fc4f5b45250513199f46f352b7e4669e");

        #ifdef ENERGI_MINE_NEW_GENESIS_BLOCK
        if (consensus.hashGenesisBlock != expectedGenesisHash)
        {
            GenesisMiner mine(genesis, strNetworkID);
        }
        #endif // ENERGI_MINE_NEW_GENESIS_BLOCK

        assert(valid_genesis_pow);
        assert(consensus.hashGenesisBlock == expectedGenesisHash);
        assert(genesis.hashMerkleRoot == expectedGenesisMerkleRoot);

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fAllowMultipleAddressesFromGroup = true;
        fAllowMultiplePorts = true;

        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        // See https://github.com/dashpay/dash/pull/1969
        // Energi prefix: Base58 't' = 127 = 0x7F
        // privKey: cP6fF1kWCPWWWDjKpFzkCPJL6rUWVhErc2uBxBMivLv7fRRDcDBK
        strSporkAddress = "tCri6YknwQ4wnUQGxqDxSb6hdMaU7rPR3z";

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("0x378abe3d42888769177494063edd42e6c3925e938ff8f73c71a6b6ad5b293ea7"))

            // Blacklist
            ,
            {}
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };
        // Testnet Energi addresses start with 't'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,127);
        // Testnet Energi script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Testnet Energi BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        // Testnet Energi BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Dedicated NRG testnet for BIP44
        nExtCoinType = 19797;
        // BIP44 test coin type is '1' (All coin's testnet default)
        nLegacyExtCoinType = 1;

        nStakeMinAge = 0;
   }

    void UpdateBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
            return mainParams;
    else if (chain == CBaseChainParams::TESTNET)
            return testNetParams;
#ifdef ENERGI_ENABLE_TESTNET_60X
    else if (chain == CBaseChainParams::TESTNET60X)
            return testNet60xParams;
#endif
    else if (chain == CBaseChainParams::DEVNET) {
            assert(devNetParams);
            return *devNetParams;
    }
    else if (chain == CBaseChainParams::REGTEST)
            return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    if (network == CBaseChainParams::DEVNET) {
        devNetParams = new CDevNetParams();
    }

    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

void UpdateRegtestBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    regTestParams.UpdateBIP9Parameters(d, nStartTime, nTimeout);
}
