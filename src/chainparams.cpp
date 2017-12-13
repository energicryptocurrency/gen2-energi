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

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"
#include "arith_uint256.h"

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nHeight  = 0;
    genesis.hashMix.SetNull();
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(txNew);
    genesis.hashPrevBlock.SetNull();
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
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "World Power";
    const CScript genesisOutputScript = CScript() << ParseHex("04494295bcacec9dad5aa01f28183f1f27e088cf7e950e21160d2f5eaad024a34eff1112f5cf3bd0fc80754e5cd4a26fde9c6866959e449a5990782c8b60d5f4f5") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
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
                cout << "Mined genesis block for " << networkID << " network: " << genesisBlock.GetHash().ToString() << endl
                    << "target was " << bnTarget.ToString() << " POWHash was " << genesisBlock.GetPOWHash().ToString() << endl
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
        auto const & seedhash = seedhash_to_filename(get_seedhash(0));

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
        consensus.foundersAddress = "TODO: implement me";
        // ~23.15 energi = 23.148148148148148 = 1000_000 / ( 30 * 24 * 60 )
        // 23.15  energi = 23.15 * 30 * 24 * 60 -> 1000080
        // 23.15 * 10% = 2.315
        // 23.15 * 20% = 4.630
        // 23.15 * 30% = 6.945
        // 23.15 * 40% = 9.260
        consensus.nBlockSubsidy = 2315000000;
        // 10% founders reward
        consensus.nBlockSubsidyFounders = 231500000;
        // 20% miners
        consensus.nBlockSubsidyMiners = 463000000;
        // 30% masternodes
        // each masternode is paid serially.. more the master nodes more is the wait for the payment
        // masternode payment gap is masternodes minutes
        consensus.nBlockSubsidyMasternodes = 694500000;
        // 40% treasury
        consensus.nBlockSubsidyTreasury = 926000000;

        // ensure the sum of the block subsidy parts equals the whole block subsidy
        assert(consensus.nBlockSubsidyFounders + consensus.nBlockSubsidyMiners + consensus.nBlockSubsidyMasternodes + consensus.nBlockSubsidyTreasury == consensus.nBlockSubsidy);

        consensus.nSubsidyHalvingInterval = 210240; /* Older value */ // Note: actual number of blocks per calendar year with DGW v3 is ~200700 (for example 449750 - 249050)
        consensus.nSubsidyHalvingInterval = 41540; /*  */

        consensus.nMasternodePaymentsStartBlock = 87600; // should be about 2 months after genesis
        consensus.nMasternodePaymentsIncreaseBlock = 158000; // actual historical value
        consensus.nMasternodePaymentsIncreasePeriod = 576*30; // 17280 - actual historical value
        consensus.nInstantSendKeepLock = 24;

        consensus.nBudgetPaymentsStartBlock = 0; // actual historical value
        consensus.nBudgetPaymentsCycleBlocks = 16616; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725
        consensus.nBudgetPaymentsWindowBlocks = 100;
        consensus.nBudgetProposalEstablishingTime = 60*60*24; // 1 day
        consensus.nSuperblockStartBlock = 0; //
        consensus.nSuperblockCycle = 16616; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725

        consensus.nGovernanceMinQuorum = 10;
        consensus.nGovernanceFilterElements = 20000;

        consensus.nMasternodeMinimumConfirmations = 15;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.powLimit = uint256S("00000fffff000000000000000000000000000000000000000000000000000000");

        consensus.nPowTargetTimespan = 24 * 60 * 60; // Dash: 1 day
        consensus.nPowTargetSpacing = 60; // Energi: 1 minute
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1486252800; // Feb 5th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1517788800; // Feb 5th, 2018

        // Deployment of DIP0001
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1508025600; // Oct 15th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1539561600; // Oct 15th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 4032;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 3226; // 80% of 4032

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000172210fe351643b3f1"); // 750000

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00000000000000b4181bbbdddbae464ce11fede5d0292fb63fdede1e7c8ab21c"); //750000

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xec;
        pchMessageStart[1] = 0x2d;
        pchMessageStart[2] = 0x9a;
        pchMessageStart[3] = 0xaf;
        vAlertPubKey = ParseHex("0466e98d9fe73af302d93ead0a7637834837e04c04fd67c9f100b278b53f004737b944f7caf8d32e6a2e1e368df31a44992b93569726d795bdc65dca439c8eec47");
        nDefaultPort = 9797;
        nMaxTipAge = 6 * 60 * 60; // ~144 blocks behind -> 2 x fork detection time, was 24 * 60 * 60 in bitcoin
        nDelayGetHeadersTime = 24 * 60 * 60;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1390095618, 28917698, 0x1e0ffff0, 1, consensus.nBlockSubsidy);
        consensus.hashGenesisBlock = genesis.GetHash();

        // TODO: mine genesis block for main net
        //uint256 expectedGenesisHash = uint256S("0x440cbbe939adba25e9e41b976d3daf8fb46b5f6ac0967b0a9ed06a749e7cf1e2");
        //uint256 expectedGenesisMerkleRoot = uint256S("0x6a4855e61ae0da5001564cee6ba8dcd7bc361e9bb12e76b62993390d6db25bca");

        //#ifdef ENERGI_MINE_NEW_GENESIS_BLOCK
        //if (consensus.hashGenesisBlock != expectedGenesisHash)
        //{
        //    GenesisMiner mine(genesis, strNetworkID);
        //}
        //#endif // ENERGI_MINE_NEW_GENESIS_BLOCK
//
        //assert(consensus.hashGenesisBlock == expectedGenesisHash);
        //assert(genesis.hashMerkleRoot == expectedGenesisMerkleRoot);

        // BIP34 is always active in Energi
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = consensus.hashGenesisBlock;

        vSeeds.push_back(CDNSSeedData("energi.network", "us-seed1.energi.network"));

        // Energi addresses start with 'E'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,33);
        // Energi script addresses start with '3'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        // Energi private keys start with e'
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,92);
        // Energi BIP32 pubkeys start with 'xpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        // Energi BIP32 prvkeys start with 'xprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        // Energi BIP44 coin type is '5'
        nExtCoinType = 5;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = false;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour
        // TODO change the public keys
        strSporkPubKey = "049b34e6fb9eb999228f984a96d05ca6556e0d09f1c2fc069fecc514dac885cf13012324531acbb07be1c517359abd3855e98ebc133edbda47e2449292eac47325";
        strMasternodePaymentsPubKey = "04ead8cb682e9df4d464539821b43e9a4a14015d5402f6884e632b6a4a2338ee3970d0513620e2686dc8261b4c64c427dc8563edcbcfbd3382ad295e8fa71e1e5f";

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (  1500, uint256S("0x000000aaf0300f59f49bc3e970bad15c11f961fe2347accffff19d96ec9778e3"))
            (  4991, uint256S("0x000000003b01809551952460744d5dbb8fcbd6cbae3c220267bf7fa43f837367"))
            (  9918, uint256S("0x00000000213e229f332c0ffbe34defdaa9e74de87f2d8d1f01af8d121c3c170b"))
            ( 16912, uint256S("0x00000000075c0d10371d55a60634da70f197548dbbfa4123e12abfcbc5738af9"))
            ( 23912, uint256S("0x0000000000335eac6703f3b1732ec8b2f89c3ba3a7889e5767b090556bb9a276"))
            ( 35457, uint256S("0x0000000000b0ae211be59b048df14820475ad0dd53b9ff83b010f71a77342d9f"))
            ( 45479, uint256S("0x000000000063d411655d590590e16960f15ceea4257122ac430c6fbe39fbf02d"))
            ( 55895, uint256S("0x0000000000ae4c53a43639a4ca027282f69da9c67ba951768a20415b6439a2d7"))
            ( 68899, uint256S("0x0000000000194ab4d3d9eeb1f2f792f21bb39ff767cb547fe977640f969d77b7"))
            ( 74619, uint256S("0x000000000011d28f38f05d01650a502cc3f4d0e793fbc26e2a2ca71f07dc3842"))
            ( 75095, uint256S("0x0000000000193d12f6ad352a9996ee58ef8bdc4946818a5fec5ce99c11b87f0d"))
            ( 88805, uint256S("0x00000000001392f1652e9bf45cd8bc79dc60fe935277cd11538565b4a94fa85f"))
            ( 107996, uint256S("0x00000000000a23840ac16115407488267aa3da2b9bc843e301185b7d17e4dc40"))
            ( 137993, uint256S("0x00000000000cf69ce152b1bffdeddc59188d7a80879210d6e5c9503011929c3c"))
            ( 167996, uint256S("0x000000000009486020a80f7f2cc065342b0c2fb59af5e090cd813dba68ab0fed"))
            ( 207992, uint256S("0x00000000000d85c22be098f74576ef00b7aa00c05777e966aff68a270f1e01a5"))
            ( 312645, uint256S("0x0000000000059dcb71ad35a9e40526c44e7aae6c99169a9e7017b7d84b1c2daf"))
            ( 407452, uint256S("0x000000000003c6a87e73623b9d70af7cd908ae22fee466063e4ffc20be1d2dbc"))
            ( 523412, uint256S("0x000000000000e54f036576a10597e0e42cc22a5159ce572f999c33975e121d4d"))
            ( 523930, uint256S("0x0000000000000bccdb11c2b1cfb0ecab452abf267d89b7f46eaf2d54ce6e652c"))
            ( 750000, uint256S("0x00000000000000b4181bbbdddbae464ce11fede5d0292fb63fdede1e7c8ab21c")),
            1507424630, // * UNIX timestamp of last checkpoint block
            3701128,    // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            5000        // * estimated number of transactions per day after checkpoint
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
        consensus.foundersAddress = "tFLyidSoz9teKks22hscftwhVHqdewvAzY";
        // ~23.15 energi = 23.148148148148148 = 1000_000 / ( 30 * 24 * 60 )
        // 23.15  energi = 23.15 * 30 * 24 * 60 -> 1000080
        // 23.15 * 10% = 2.315
        // 23.15 * 20% = 4.630
        // 23.15 * 30% = 6.945
        // 23.15 * 40% = 9.260
        consensus.nBlockSubsidy = 2315000000;
        // 10% founders reward
        consensus.nBlockSubsidyFounders = 231500000;
        // 20% miners
        consensus.nBlockSubsidyMiners = 463000000;
        // 30% masternodes
        // each masternode is paid serially.. more the master nodes more is the wait for the payment
        // masternode payment gap is masternodes minutes
        consensus.nBlockSubsidyMasternodes = 694500000;
        // 40% treasury
        consensus.nBlockSubsidyTreasury = 926000000;

        // ensure the sum of the block subsidy parts equals the whole block subsidy
        assert(consensus.nBlockSubsidyFounders + consensus.nBlockSubsidyMiners + consensus.nBlockSubsidyMasternodes + consensus.nBlockSubsidyTreasury == consensus.nBlockSubsidy);

        // TODO: fix value for this parameter
        consensus.nSubsidyHalvingInterval = 210240; /* Older value */ // Note: actual number of blocks per calendar year with DGW v3 is ~200700 (for example 449750 - 249050)
        consensus.nSubsidyHalvingInterval = 41540; /*  */

        consensus.nMasternodePaymentsStartBlock = 87600; // should be about 2 months after genesis
        consensus.nMasternodePaymentsIncreaseBlock =  46000;
        consensus.nMasternodePaymentsIncreasePeriod = 576;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 4100;
        consensus.nBudgetPaymentsCycleBlocks = 50;
        consensus.nBudgetPaymentsWindowBlocks = 10;
        consensus.nBudgetProposalEstablishingTime = 60*20;
        consensus.nSuperblockStartBlock = 61000; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPeymentsStartBlock
        consensus.nSuperblockCycle = 24;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.nMajorityEnforceBlockUpgrade = 51;
        consensus.nMajorityRejectBlockOutdated = 75;
        consensus.nMajorityWindow = 100;
        consensus.powLimit = uint256S("00000fffff000000000000000000000000000000000000000000000000000000");
        consensus.nPowTargetTimespan = 24 * 60 * 60; // in seconds -> Energi: 1 day
        consensus.nPowTargetSpacing = 60; // in seconds Energi: 1 minute
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1486252800; // Feb 5th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1517788800; // Feb 5th, 2018

        pchMessageStart[0] = 0xd9;
        pchMessageStart[1] = 0x2a;
        pchMessageStart[2] = 0xab;
        pchMessageStart[3] = 0x6e;
        vAlertPubKey = ParseHex("04c3660259aa76854c135c5b5b0a668cecc7ca81b531dbbb212353c90132ba32cf1a11309eec989856bbf1748d047b69590279633f15e5e9835d263acdedad2400");
        nDefaultPort = 19797;
        nMaxTipAge = 0x7fffffff; // allow mining on top of old blocks for testnet
        nDelayGetHeadersTime = 24 * 60 * 60;
        nPruneAfterHeight = 1000;


        genesis = CreateGenesisBlock(1512083591UL, 2537147, 0x1e0ffff0, 1, consensus.nBlockSubsidy);
        consensus.hashGenesisBlock = genesis.GetHash();

        uint256 expectedGenesisHash = uint256S("0xd1f7327e61b34d7660e1bc8bf8fe202f86bf84f1847110309e89dfc9e193d3ae");
        uint256 expectedGenesisMerkleRoot = uint256S("0x398bc532d10d33bb8ed323860572558ba89ccecadfd33b0f7a25816df65cda6d");

        // TODO: mine genesis block for testnet
        #ifdef ENERGI_MINE_NEW_GENESIS_BLOCK
        if (consensus.hashGenesisBlock != expectedGenesisHash)
        {
            GenesisMiner mine(genesis, strNetworkID);
        }
        #endif // ENERGI_MINE_NEW_GENESIS_BLOCK

        assert(consensus.hashGenesisBlock == expectedGenesisHash);
        assert(genesis.hashMerkleRoot == expectedGenesisMerkleRoot);

        // BIP34 is always active in Energi
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = consensus.hashGenesisBlock;

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("energi.network", "us-seed1.test.energi.network"));

        // Testnet Energi addresses start with 't'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,127);
        // Testnet Dash script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Testnet Dash BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        // Testnet Dash BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Testnet Dash BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
        strSporkPubKey = "046f8bd5301f95b88d75f3557c732785e12916930506406e8919389f5360e7dffefbe6d6e45144b9ab837d65e21ab93b67d4af30d27ba38c5e5622d31903338039";
        strMasternodePaymentsPubKey = "040c342b5f1a50c6b29adb92c5105eb8a1bea5151c7d353d30b85314a86bd577f1a76e27372220233a54afbb61b7c8f0e7e7dc6a030dc1b801b71369618ba59961";

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
static CTestNetParams testNetParams;


/**
 * Testnet (60x)
 */
class CTestNet60xParams : public CChainParams {
public:
    CTestNet60xParams() {
        strNetworkID = "test60";

        // Energi distribution parameters
        consensus.foundersAddress = "tFLyidSoz9teKks22hscftwhVHqdewvAzY";
        // ~23.15 energi = 23.148148148148148 = 1000_000 / ( 30 * 24 * 60 )
        // 23.15  energi = 23.15 * 30 * 24 * 60 -> 1000080
        // 23.15 * 10% = 2.315
        // 23.15 * 20% = 4.630
        // 23.15 * 30% = 6.945
        // 23.15 * 40% = 9.260
        consensus.nBlockSubsidy = 138900000000; //2315000000 * 60;
        // 10% founders reward
        consensus.nBlockSubsidyFounders = 13890000000;
        // 20% miners
        consensus.nBlockSubsidyMiners = 27780000000;
        // 30% masternodes
        // each masternode is paid serially.. more the master nodes more is the wait for the payment
        // masternode payment gap is masternodes minutes
        consensus.nBlockSubsidyMasternodes = 41670000000;
        // 40% treasury
        consensus.nBlockSubsidyTreasury = 55560000000;

        // ensure the sum of the block subsidy parts equals the whole block subsidy
        assert(consensus.nBlockSubsidyFounders + consensus.nBlockSubsidyMiners + consensus.nBlockSubsidyMasternodes + consensus.nBlockSubsidyTreasury == consensus.nBlockSubsidy);

        // TODO: fix value for this parameter
        consensus.nSubsidyHalvingInterval = 210240; /* Older value */ // Note: actual number of blocks per calendar year with DGW v3 is ~200700 (for example 449750 - 249050)
        consensus.nSubsidyHalvingInterval = 41540; /*  */

        consensus.nMasternodePaymentsStartBlock = 1460; // 87600/60.. little over a day
        consensus.nMasternodePaymentsIncreaseBlock =  0;
        consensus.nMasternodePaymentsIncreasePeriod = 0;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 4320; // after 3 days
        consensus.nBudgetPaymentsCycleBlocks = 50;
        consensus.nBudgetPaymentsWindowBlocks = 10;
        consensus.nBudgetProposalEstablishingTime = 60*20;
        consensus.nSuperblockStartBlock = 6100; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPaymentsStartBlock
        consensus.nSuperblockCycle = 24;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.nMajorityEnforceBlockUpgrade = 51;
        consensus.nMajorityRejectBlockOutdated = 75;
        consensus.nMajorityWindow = 100;
        consensus.powLimit = uint256S("00000fffff000000000000000000000000000000000000000000000000000000");
        consensus.nPowTargetTimespan = 24 * 60 * 60; // in seconds -> Energi: 1 day
        consensus.nPowTargetSpacing = 60; // in seconds Energi: 1 minute
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
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
        vAlertPubKey = ParseHex("04c3660259aa76854c135c5b5b0a668cecc7ca81b531dbbb212353c90132ba32cf1a11309eec989856bbf1748d047b69590279633f15e5e9835d263acdedad2400");
        nDefaultPort = 29797;
        nMaxTipAge = 0x7fffffff; // allow mining on top of old blocks for testnet
        nDelayGetHeadersTime = 24 * 60 * 60;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1512083684UL, 36291660, 0x1e0ffff0, 1, consensus.nBlockSubsidy);
        consensus.hashGenesisBlock = genesis.GetHash();
        uint256 expectedGenesisHash = uint256S("0x69e6c1873b72fba2fed2e203ad3fcf1412ff0a107770273b2b6b44672ff12177");
        uint256 expectedGenesisMerkleRoot = uint256S("0xf5be3fa001329f6c2154bae1416e8194a570d9ecfb0cf78e8bdee1fefc739cf5");

        // TODO: mine genesis block for testnet60x
        #ifdef ENERGI_MINE_NEW_GENESIS_BLOCK
        if (consensus.hashGenesisBlock != expectedGenesisHash)
        {
            GenesisMiner mine(genesis, strNetworkID);
        }
        #endif // ENERGI_MINE_NEW_GENESIS_BLOCK

        assert(consensus.hashGenesisBlock == expectedGenesisHash);
        assert(genesis.hashMerkleRoot == expectedGenesisMerkleRoot);

        // BIP34 is always active in Energi
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = consensus.hashGenesisBlock;

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("energi.network",  "us-seed1.test60x.energi.network"));

        // Testnet Energi addresses start with 't'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,127);
        // Testnet Dash script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Testnet Dash BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        // Testnet Dash BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Testnet Dash BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test60x, pnSeed6_test60x + ARRAYLEN(pnSeed6_test60x));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
        strSporkPubKey = "046f8bd5301f95b88d75f3557c732785e12916930506406e8919389f5360e7dffefbe6d6e45144b9ab837d65e21ab93b67d4af30d27ba38c5e5622d31903338039";
        strMasternodePaymentsPubKey = "040c342b5f1a50c6b29adb92c5105eb8a1bea5151c7d353d30b85314a86bd577f1a76e27372220233a54afbb61b7c8f0e7e7dc6a030dc1b801b71369618ba59961";

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

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";

        // Energi distribution parameters
        consensus.foundersAddress = "tFLyidSoz9teKks22hscftwhVHqdewvAzY";
        // ~23.15 energi = 23.148148148148148 = 1000_000 / ( 30 * 24 * 60 )
        // 23.15  energi = 23.15 * 30 * 24 * 60 -> 1000080
        // 23.15 * 10% = 2.315
        // 23.15 * 20% = 4.630
        // 23.15 * 30% = 6.945
        // 23.15 * 40% = 9.260
        consensus.nBlockSubsidy = 2315000000;
        // 10% founders reward
        consensus.nBlockSubsidyFounders = 231500000;
        // 20% miners
        consensus.nBlockSubsidyMiners = 463000000;
        // 30% masternodes
        // each masternode is paid serially.. more the master nodes more is the wait for the payment
        // masternode payment gap is masternodes minutes
        consensus.nBlockSubsidyMasternodes = 694500000;
        // 40% treasury
        consensus.nBlockSubsidyTreasury = 926000000;

        // ensure the sum of the block subsidy parts equals the whole block subsidy
        assert(consensus.nBlockSubsidyFounders + consensus.nBlockSubsidyMiners + consensus.nBlockSubsidyMasternodes + consensus.nBlockSubsidyTreasury == consensus.nBlockSubsidy);

        // TODO: are consensus.nSubsidyHalvingInterval params needed?

        consensus.nMasternodePaymentsStartBlock = 240;
        consensus.nMasternodePaymentsIncreaseBlock = 350;
        consensus.nMasternodePaymentsIncreasePeriod = 10;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 1000;
        consensus.nBudgetPaymentsCycleBlocks = 50;
        consensus.nBudgetPaymentsWindowBlocks = 10;
        consensus.nBudgetProposalEstablishingTime = 60*20;
        consensus.nSuperblockStartBlock = 1500;
        consensus.nSuperblockCycle = 10;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 100;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = -1; // BIP34 has not necessarily activated on regtest
        consensus.BIP34Hash = uint256();
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Dash: 1 day
        consensus.nPowTargetSpacing = 60; // Energi: 1 minute
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 999999999999ULL;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xef;
        pchMessageStart[1] = 0x89;
        pchMessageStart[2] = 0x6c;
        pchMessageStart[3] = 0x7f;
        nMaxTipAge = 6 * 60 * 60; // ~144 blocks behind -> 2 x fork detection time, was 24 * 60 * 60 in bitcoin
        nDelayGetHeadersTime = 0; // never delay GETHEADERS in regtests
        nDefaultPort = 39797;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1512083866UL, 3, 0x207fffff, 1, consensus.nBlockSubsidy);
        consensus.hashGenesisBlock = genesis.GetHash();

        uint256 expectedGenesisHash = uint256S("0x498c712d8bd64ab5660b8bfe83901adfd79e3276c829e43ce63ec85f56ff3b0e");
        uint256 expectedGenesisMerkleRoot = uint256S("0x398bc532d10d33bb8ed323860572558ba89ccecadfd33b0f7a25816df65cda6d");

        #ifdef ENERGI_MINE_NEW_GENESIS_BLOCK
        if (consensus.hashGenesisBlock != expectedGenesisHash)
        {
            GenesisMiner mine(genesis, strNetworkID);
        }
        #endif // ENERGI_MINE_NEW_GENESIS_BLOCK

        assert(consensus.hashGenesisBlock == expectedGenesisHash);
        assert(genesis.hashMerkleRoot == expectedGenesisMerkleRoot);
        vFixedSeeds.clear(); //! Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();  //! Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;

        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("0x440cbbe939adba25e9e41b976d3daf8fb46b5f6ac0967b0a9ed06a749e7cf1e2")),
            0,
            0,
            0
        };
        // Regtest Dash addresses start with 'y'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
        // Regtest Dash script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Regtest private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Regtest Dash BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        // Regtest Dash BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Regtest Dash BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;
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
        else if (chain == CBaseChainParams::TESTNET60X)
            return testNet60xParams;
    else if (chain == CBaseChainParams::REGTEST)
            return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}
