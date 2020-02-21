// Copyright (c) 2019-2019 The Energi Core developers
// Distributed under the MIT software license, see the accompanying
/* @flow */
// Copyright (c) 2012-2013 The PPCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>

#include "chainparams.h"
#include "db.h"
#include "pos_kernel.h"
#include "script/interpreter.h"
#include "policy/policy.h"
#include "timedata.h"
#include "util.h"
#include "consensus/validation.h"
#include "spork.h"

using namespace std;

// NOTE: This should be > 10 minutes
// MODIFIER_INTERVAL: time to elapse before new modifier is computed
static constexpr unsigned int MODIFIER_INTERVAL = 60;

// MODIFIER_INTERVAL_RATIO:
// ratio of group interval length between the last group and the first group
static constexpr int MODIFIER_INTERVAL_RATIO = 3;

static constexpr int MODIFIER_INTERVAL_SECTIONS_MAX = 64;

// Get the last stake modifier and its generation time from a given block
static bool GetLastStakeModifier(const CBlockIndex* pindex, uint64_t& nStakeModifier, int64_t& nModifierTime)
{
    if (!pindex)
        return error("GetLastStakeModifier: null pindex");
    while (pindex && pindex->pprev && !pindex->IsGeneratedStakeModifier())
        pindex = pindex->pprev;
    nStakeModifier = pindex->nStakeModifier();
    nModifierTime = pindex->GetBlockTime();
    return true;
}

template <int Section>
struct StakeModifierSelectionIntervalSection {
    static_assert(Section >= 0 && Section < MODIFIER_INTERVAL_SECTIONS_MAX, "Invalid nSection");
    static constexpr auto MI_1 = MODIFIER_INTERVAL_SECTIONS_MAX - 1;
    static constexpr int value = (MODIFIER_INTERVAL * MI_1) / (MI_1 + ((MI_1 - Section) * (MODIFIER_INTERVAL_RATIO - 1)));
};

using IntervalSectionArray = int64_t[64];

template<int Section>
struct StakeModifierSelectionIntervalHelper {
    static constexpr int64_t value = (
        StakeModifierSelectionIntervalHelper<Section-1>::value
        + StakeModifierSelectionIntervalSection<Section>::value
    );
    static void fill(IntervalSectionArray &arr) {
        arr[Section] = StakeModifierSelectionIntervalSection<Section>::value;
        StakeModifierSelectionIntervalHelper<Section-1>::fill(arr);
    }
};
template<>
struct StakeModifierSelectionIntervalHelper<0> {
    static constexpr int64_t value = StakeModifierSelectionIntervalSection<0>::value;
    static void fill(IntervalSectionArray &arr) {
        arr[0] = StakeModifierSelectionIntervalSection<0>::value;
    }
};

class StakeModifierIntervalSelector {
private:
    IntervalSectionArray array;

public:
    StakeModifierIntervalSelector() {
        StakeModifierSelectionIntervalHelper<MODIFIER_INTERVAL_SECTIONS_MAX - 1>::fill(array);
    }
    
    int64_t Get(int section) {
        assert(section >= 0 && section < 64);
        return array[section];
    }
};
StakeModifierIntervalSelector g_StakeModifierIntervalSelector;

// Get stake modifier selection interval (in seconds)
static constexpr int64_t GetStakeModifierSelectionInterval()
{
    return StakeModifierSelectionIntervalHelper<MODIFIER_INTERVAL_SECTIONS_MAX - 1>::value;
}

// select a block from the candidate blocks in vSortedByTimestamp, excluding
// already selected blocks in vSelectedBlocks, and with timestamp up to
// nSelectionIntervalStop.
static bool SelectBlockFromCandidates(
    vector<pair<int64_t, uint256> >& vSortedByTimestamp,
    map<uint256, const CBlockIndex*>& mapSelectedBlocks,
    int64_t nSelectionIntervalStop,
    uint64_t nStakeModifierPrev,
    const CBlockIndex** pindexSelected)
{
    bool fSelected = false;
    arith_uint256 hashBest{0};
    *pindexSelected = nullptr;

    for (auto iter = vSortedByTimestamp.begin(); ; ++iter) {
        if (iter == vSortedByTimestamp.end()) {
            vSortedByTimestamp.clear();
            break;
        }

        if (!mapBlockIndex.count(iter->second))
            return error("SelectBlockFromCandidates: failed to find block index for candidate block %s", iter->second.ToString().c_str());

        const CBlockIndex* pindex = mapBlockIndex[iter->second];
        if (fSelected && pindex->GetBlockTime() > nSelectionIntervalStop) {
            // No point to re-consider the blocks
            vSortedByTimestamp.erase(vSortedByTimestamp.begin(), iter+1);
            break;
        }

        if (mapSelectedBlocks.count(pindex->GetBlockHash()) > 0)
            continue;

        // compute the selection hash by hashing an input that is unique to that block
        uint256 hashProof = pindex->GetBlockHash();

        CDataStream ss(SER_GETHASH, 0);
        ss << hashProof << nStakeModifierPrev;
        arith_uint256 hashSelection = UintToArith256(Hash(ss.begin(), ss.end()));

        // the selection hash is divided by 2**32 so that proof-of-stake block
        // is always favored over proof-of-work block. this is to preserve
        // the energy efficiency property
        if (pindex->IsProofOfStake())
            hashSelection >>= 32;

        if (fSelected && hashSelection < hashBest) {
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*)pindex;
        } else if (!fSelected) {
            iter = vSortedByTimestamp.begin();
            fSelected = true;
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*)pindex;
        }
    }

    LogPrint("stake", "%s: selection hash=%s\n", __func__, hashBest.ToString().c_str());

    return fSelected;
}

// Stake Modifier (hash modifier of proof-of-stake):
// The purpose of stake modifier is to prevent a txout (coin) owner from
// computing future proof-of-stake generated by this txout at the time
// of transaction confirmation. To meet kernel protocol, the txout
// must hash with a future stake modifier to generate the proof.
// Stake modifier consists of bits each of which is contributed from a
// selected block of a given block group in the past.
// The selection of a block is based on a hash of the block's proof-hash and
// the previous stake modifier.
// Stake modifier is recomputed at a fixed time interval instead of every
// block. This is to make it difficult for an attacker to gain control of
// additional bits in the stake modifier, even after generating a chain of
// blocks.
bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier)
{
    nStakeModifier = 0;
    if (!pindexPrev) {
        return true; // genesis block's modifier is 0
    }
    if (pindexPrev->nHeight == 0) {
        //Give a stake modifier to the first block
        nStakeModifier = 0x1234567887654321;
        return true;
    }

    // First find current stake modifier and its generation block time
    // if it's not old enough, return the same stake modifier
    int64_t nModifierTime = 0;
    if (!GetLastStakeModifier(pindexPrev, nStakeModifier, nModifierTime))
        return error("ComputeNextStakeModifier: unable to get last modifier");

    LogPrint("stake", "%s: prev modifier=%llx time=%d\n",
             __func__, nStakeModifier, nModifierTime);

    LogPrint("stake", "%s: block %d modifier time last vs prev %llx~%llx >= %llx~%llx\n",
             __func__, pindexPrev->nHeight,
             nModifierTime, (nModifierTime / MODIFIER_INTERVAL),
             pindexPrev->GetBlockTime(), (pindexPrev->GetBlockTime() / MODIFIER_INTERVAL));

    if (nModifierTime / MODIFIER_INTERVAL >= pindexPrev->GetBlockTime() / MODIFIER_INTERVAL) {
        return true;
    }

    // Sort candidate blocks by timestamp
    vector<pair<int64_t, uint256> > vSortedByTimestamp;
    vSortedByTimestamp.reserve(MODIFIER_INTERVAL_SECTIONS_MAX);
    int64_t nSelectionInterval = GetStakeModifierSelectionInterval();
    int64_t nSelectionIntervalStart = (pindexPrev->GetBlockTime() / MODIFIER_INTERVAL) * MODIFIER_INTERVAL - nSelectionInterval;
    const CBlockIndex* pindex = pindexPrev;

    while (pindex && pindex->GetBlockTime() >= nSelectionIntervalStart) {
        vSortedByTimestamp.push_back(make_pair(pindex->GetBlockTime(), pindex->GetBlockHash()));
        pindex = pindex->pprev;
    }

    int nHeightFirstCandidate = pindex ? (pindex->nHeight + 1) : 0;
    reverse(vSortedByTimestamp.begin(), vSortedByTimestamp.end()); // it should improve sorting time
    sort(vSortedByTimestamp.begin(), vSortedByTimestamp.end());

    // Select 64 blocks from candidate blocks to generate stake modifier
    uint64_t nStakeModifierNew = 0;
    int64_t nSelectionIntervalStop = nSelectionIntervalStart;
    map<uint256, const CBlockIndex*> mapSelectedBlocks;
    for (int nRound = 0; nRound < MODIFIER_INTERVAL_SECTIONS_MAX; nRound++) {
        // add an interval section to the current selection round
        nSelectionIntervalStop += g_StakeModifierIntervalSelector.Get(nRound);

        // select a block from the candidates of current round
        if (SelectBlockFromCandidates(vSortedByTimestamp, mapSelectedBlocks, nSelectionIntervalStop, nStakeModifier, &pindex)) {
            // write the entropy bit of the selected block
            nStakeModifierNew |= (((uint64_t)pindex->GetStakeEntropyBit()) << nRound);
        } else {
            LogPrint("stake", "WARN: unable to select candidate block for stake modifier at round %d - left zero\n", nRound);
            break;
        }

        // add the selected block from candidates to selected list
        mapSelectedBlocks.insert(make_pair(pindex->GetBlockHash(), pindex));
        LogPrint("stake", "%s: selected round %d stop=%s height=%d bit=%d\n",
                 __func__, nRound, nSelectionIntervalStop,
                 pindex->nHeight, pindex->GetStakeEntropyBit());
    }

    // Print selection map for visualization of the selected blocks
    if (LogAcceptCategory("stakemod")) {
        string strSelectionMap = "";
        // '-' indicates proof-of-work blocks not selected
        strSelectionMap.insert(0, pindexPrev->nHeight - nHeightFirstCandidate + 1, '-');
        pindex = pindexPrev;
        while (pindex && pindex->nHeight >= nHeightFirstCandidate) {
            // '=' indicates proof-of-stake blocks not selected
            if (pindex->IsProofOfStake())
                strSelectionMap.replace(pindex->nHeight - nHeightFirstCandidate, 1, "=");
            pindex = pindex->pprev;
        }
        for (const auto &item : mapSelectedBlocks) {
            // 'S' indicates selected proof-of-stake blocks
            // 'W' indicates selected proof-of-work blocks
            strSelectionMap.replace(item.second->nHeight - nHeightFirstCandidate, 1, item.second->IsProofOfStake() ? "S" : "W");
        }
        LogPrintf("%s: selection height [%d, %d] map %s\n", __func__, nHeightFirstCandidate, pindexPrev->nHeight, strSelectionMap.c_str());
    }
    
    LogPrint("stake", "%s: new modifier=%llx prevblktime=%d\n",
             __func__, nStakeModifierNew, pindexPrev->GetBlockTime());

    nStakeModifier = nStakeModifierNew;
    return true;
}

bool ComputeNextStakeModifierV2(const uint32_t blockTime, const CBlockIndex* pindexPrev, uint64_t& nStakeModifier)
{
    nStakeModifier = 0;

    if (!pindexPrev) {
        return false;
    }

    /**
     * This is actually bound to stake maturity period of 1 hour, but testnet uses only 3 minutes.
     *
     * The previous block stake is used as difficult to mine entropy as it is expected to be outside
     * of the interval of prediction.
     *
     * The general approach is to use the oldest block in such interval as source of random entropy,
     * but block time can be manipulated.
     *
     * Therefore, the middle block time is used as another source of entropy as extra precaution.
     *
     * The problem becomes as follows:
     * - stakes needs to be mined in before maturity period
     * - target stakes depend on block hashes inside the maturity period
     * - there are around 60 blocks = 60 stakes needed
     * - oldest block hash is not predictable in the future
     * - all this problem solving needs to be sorted in quite short period of time as the main chain
     *   progresses rapidly.
     * - current hash power demand is in Thz
     */
    const uint32_t MODIFIER_INTERVAL = 3600;
    const uint32_t timeMid = blockTime - (MODIFIER_INTERVAL / 2);
    const uint32_t timeOld = blockTime - (MODIFIER_INTERVAL - MAX_POS_BLOCK_AHEAD_TIME);

    const CBlockIndex* pmiddle = pindexPrev;
    for (; pmiddle->pprev && (pmiddle->pprev->nTime > timeMid); pmiddle = pmiddle->pprev);

    const CBlockIndex* poldest = pmiddle;
    for (; poldest->pprev && (poldest->pprev->nTime > timeOld); poldest = poldest->pprev);

    auto prevout = pindexPrev->StakeInput();

    // From more recent to more oldest
    CDataStream ss(SER_GETHASH, 0);
    ss << prevout.hash << prevout.n;
    ss << pmiddle->GetBlockHash();
    ss << poldest->GetBlockHash();

    // The first 64 bits
    nStakeModifier = Hash(ss.begin(), ss.end()).GetCheapHash();

    LogPrint("stake", "%s: new modifier=%llx time=%llu, prevblk=%s\n",
             __func__, nStakeModifier,
             blockTime,
             pindexPrev->GetBlockHash().ToString().c_str());

    return true;
}

bool CachedNextStakeModifierV2(const uint32_t blockTime, const CBlockIndex* pindexPrev, uint64_t& nStakeModifier) 
{
    // This can be done more "correct" way...
    static uint256 last_parent;
    static std::map<uint32_t, uint64_t> sm_cache;

    auto parent = pindexPrev->GetBlockHash();

    // Reset cache
    if (last_parent != parent) {
        sm_cache.clear();
        last_parent = parent;
    }

    // Fast path
    auto cached = sm_cache.find(blockTime);

    if (cached != sm_cache.end()) {
        nStakeModifier = cached->second;

        LogPrint("stake", "%s: cached modifier=%llx time=%llu, prevblk=%s\n",
                __func__, nStakeModifier,
                blockTime,
                pindexPrev->GetBlockHash().ToString().c_str());
        return true;
    }

    // Slow path
    if (!ComputeNextStakeModifierV2(blockTime, pindexPrev, nStakeModifier)) {
        return false;
    }

    // Cache
    sm_cache[blockTime] = nStakeModifier;

    return true;
}

uint256 stakeHash(unsigned int nTimeTx, CDataStream ss, unsigned int prevoutIndex, uint256 prevoutHash, unsigned int nTimeBlockFrom)
{
    //Pivx will hash in the transaction hash and the index number in order to make sure each hash is unique
    ss << nTimeBlockFrom << prevoutIndex << prevoutHash << nTimeTx;
    return Hash(ss.begin(), ss.end());
}

//instead of looping outside and reinitializing variables many times, we will give a nTimeTx and also search interval so that we can do all the hashing here
bool CheckStakeKernelHash(
    CBlockHeader &current,
    const CBlockIndex &blockPrev,
    const CBlockIndex &blockFrom,
    const CTransaction txPrev,
    const COutPoint prevout,
    unsigned int nHashDrift,
    bool fCheck,
    bool fPrintProofOfStake
) {
    // Legacy way of parameter passing
    unsigned int nBits = current.nBits;
    unsigned int& nTimeTx = current.nTime;
    uint256& hashProofOfStake = current.hashProofOfStake();
    uint64_t &nStakeModifier = current.nStakeModifier();
    //

    //assign new variables to make it easier to read
    CAmount nValueIn = txPrev.vout[prevout.n].nValue;
    unsigned int nTimeBlockFrom = blockFrom.GetBlockTime();
    auto min_age = Params().MinStakeAge();
    
    if (nValueIn < MIN_STAKE_AMOUNT) {
        return error("CheckStakeKernelHash() : stake value is too small %d < %d", nValueIn, MIN_STAKE_AMOUNT);
    }

    if (nTimeTx < nTimeBlockFrom) // Transaction timestamp violation
        return error("CheckStakeKernelHash() : nTime violation");

    if (nTimeBlockFrom + min_age > nTimeTx) // Min age requirement
    {
        // During generation, some stakes may be not year ready
        if (fCheck) {
            error("%s : min age violation - nTimeBlockFrom=%d nStakeMinAge=%d nTimeTx=%d",
                  __func__, nTimeBlockFrom, min_age, nTimeTx);
        }
        return false;
    }

    //grab difficulty
    arith_uint256 bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);
    arith_uint256 bnTarget = (arith_uint256(nValueIn) / 100) * bnTargetPerCoinDay;

    if (bnTarget < bnTargetPerCoinDay) {
        LogPrint("stake", "PoS target overflow %s amount %d < common %s, using ~0\n",
                  bnTarget.GetHex().c_str(),
                  nValueIn,
                  bnTargetPerCoinDay.GetHex().c_str());
        bnTarget = ~arith_uint256(0);
    }

    //grab stake modifier
    //-------------------
    uint64_t nRequiredStakeModifier = 0;

    // This is a six month later fix of the problem stated in the note below.
    if (current.IsProofOfStakeV2()) {
        if (fCheck && !ComputeNextStakeModifierV2(nTimeTx, &blockPrev, nRequiredStakeModifier)) {
            LogPrintf("CheckStakeKernelHash(): failed to get kernel stake modifier V2 \n");
            return false;
        }
        // pass
    } else // PoS v1

    // NOTE: this must be calculated based on previous-to-tip, but not previous-to-stake block!
    if (!ComputeNextStakeModifier(&blockFrom, nRequiredStakeModifier)) {
        LogPrintf("CheckStakeKernelHash(): failed to get kernel stake modifier \n");
        return false;
    }
    
    if (fCheck) {
        if (nStakeModifier != nRequiredStakeModifier) {
            return error(
                "%s : nStakeModifier mismatch at %d %llx != %llx",
                __func__, blockFrom.nHeight,
                nStakeModifier, nRequiredStakeModifier );
        }
    } else {
        nStakeModifier = nRequiredStakeModifier;
    }

    //create data stream once instead of repeating it in the loop
    CDataStream ss(SER_GETHASH, 0);
    ss << nStakeModifier;

    // if wallet is simply checking to make sure a hash is valid
    //-------------------
    if (fCheck) {
        uint256 requiredHashProofOfStake = stakeHash(nTimeTx, ss, prevout.n, prevout.hash, nTimeBlockFrom);
        
        if (requiredHashProofOfStake != hashProofOfStake) {
            return error(
                "%s : hashProofOfStake mismatch at %d:%llx:%d:%s:%d %s != %s",
                __func__,
                nTimeTx, nStakeModifier, prevout.n, prevout.hash.ToString().c_str(), nTimeBlockFrom,
                hashProofOfStake.ToString().c_str(),
                requiredHashProofOfStake.ToString().c_str() );
        }

        return UintToArith256(hashProofOfStake) < bnTarget;
    }

    // search
    //-------------------
    auto min_time = nTimeTx;
    auto max_time = GetAdjustedTime() + std::min<int64_t>(
            nHashDrift,
            MAX_POS_BLOCK_AHEAD_TIME - MAX_POS_BLOCK_AHEAD_SAFETY_MARGIN);

    if (sporkManager.IsSporkActive(SPORK_17_BLOCK_TIME)) {
        max_time = GetAdjustedTime() + 1;
    }

    LogPrint("stake", "%s: looking for solution in range %lld .. %lld (%lld) \n",
             __func__, min_time, max_time, (max_time - min_time));

    for (auto try_time = min_time; try_time < max_time; ++try_time)
    {
        if (current.IsProofOfStakeV2()) {
            if (!CachedNextStakeModifierV2(try_time, &blockPrev, nStakeModifier)) {
                LogPrintf("CheckStakeKernelHash(): failed to get kernel stake modifier V2 \n");
                return false;
            }

            ss = CDataStream(SER_GETHASH, 0);
            ss << nStakeModifier;
        }

        //hash this iteration
        hashProofOfStake = stakeHash(try_time, ss, prevout.n, prevout.hash, nTimeBlockFrom);

        // if stake hash does not meet the target then continue to next iteration
        if (UintToArith256(hashProofOfStake) >= bnTarget) {
            continue;
        }

        nTimeTx = try_time;

        if (fDebug || fPrintProofOfStake) {
            LogPrintf("CheckStakeKernelHash() : using modifier %llx at height=%d timestamp=%s for block from height=%d timestamp=%s\n",
                nStakeModifier,
                blockFrom.nHeight,
                DateTimeStrFormat("%Y-%m-%d %H:%M:%S", blockFrom.nTime).c_str(),
                blockFrom.nHeight,
                DateTimeStrFormat("%Y-%m-%d %H:%M:%S", blockFrom.GetBlockTime()).c_str());
            LogPrintf("CheckStakeKernelHash() : pass protocol=%s modifier=%s nTimeBlockFrom=%u prevoutHash=%s nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
                "0.3",
                boost::lexical_cast<std::string>(nStakeModifier).c_str(),
                nTimeBlockFrom, prevout.hash.ToString().c_str(), nTimeBlockFrom, prevout.n, try_time,
                hashProofOfStake.ToString().c_str());
        }
        return true;
    }

    return false;
}

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(CValidationState &state, const CBlockHeader &header, const Consensus::Params& consensus)
{
    if (header.posBlockSig.empty()) {
        return state.DoS(100, false, REJECT_MALFORMED, "bad-pos-sig", false, "missing PoS signature");
    }

    COutPoint prevout = header.StakeInput();

    // First try finding the previous transaction in database
    uint256 txinHashBlock;
    CTransactionRef txinPrevRef;
    CBlockIndex* pindex_tx = nullptr;
    CBlockIndex* pindex_prev = nullptr;

    if (!GetTransaction(prevout.hash, txinPrevRef, consensus, txinHashBlock, true)) {
        BlockMap::iterator it = mapBlockIndex.find(header.hashPrevBlock);
        
        if ((it != mapBlockIndex.end()) && chainActive.Contains(it->second)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-unkown-stake");
        } else {
            // We do not have the previous block, so the block may be valid
            return state.TransientError("tmp-bad-unkown-stake");
        }
    }

    // Check tx input block is known
    {
        BlockMap::iterator it = mapBlockIndex.find(txinHashBlock);

        if ((it != mapBlockIndex.end()) && chainActive.Contains(it->second)) {
            pindex_tx = it->second;
        } else {
            it = mapBlockIndex.find(header.hashPrevBlock);
            
            if ((it != mapBlockIndex.end()) && chainActive.Contains(it->second)) {
                return state.DoS(100, false, REJECT_INVALID, "bad-stake-mempool",
                                 false, "stake from mempool");
            } else {
                // We do not have the previous block, so the block may be valid
                return state.TransientError("tmp-bad-stake-mempool");
            }
        }
    }

    // Header-only chain specific validation
    {
        BlockMap::iterator it = mapBlockIndex.find(header.hashPrevBlock);

        // It must never happen as it's part of header validation.
        if (it == mapBlockIndex.end()) {
            return state.DoS(100, false, REJECT_INVALID, "bad-prev-header",
                                false, "previous PoS header is not known");
        }

        pindex_prev = it->second;
        const auto pindex_fork = chainActive.FindFork(pindex_prev);

        // Just in case, it must never happen.
        if (!pindex_fork) {
            return state.DoS(100, false, REJECT_INVALID, "bad-fork-point",
                                false, "the fork point is not found");
        }

        // Check if UTXO is beyond possible fork point
        if (pindex_fork->nHeight < pindex_tx->nHeight) {
            return state.DoS(100, false, REJECT_INVALID, "bad-stake-after-fork",
                                false, "rogue fork tries to use UTXO from the current chain");
        }

        // Check if UTXO is used in headers before the last known fully validated block
        for (auto pindex_walk = pindex_prev;
             (pindex_walk != pindex_fork) &&
                pindex_walk->IsProofOfStake() &&
                !pindex_walk->IsValid(BLOCK_VALID_SCRIPTS);
             pindex_walk = pindex_fork->pprev
        ) {
            if (prevout == pindex_walk->StakeInput()) {
                return state.DoS(100, false, REJECT_INVALID, "bad-header-double-spent",
                                 false, "rogue fork tries use the same UTXO twice");
            }
        }
    }

    // NOTE: stake age check is part of CheckStakeKernelHash()

    // Check stake maturity (double checking with other functionality for DoS mitigation)
    if (txinPrevRef->IsCoinBase() &&
        ((header.nHeight - pindex_tx->nHeight) <= COINBASE_MATURITY)
    ) {
        return state.DoS(100, false, REJECT_INVALID, "bad-stake-coinbase-maturity",
                            false, "coinbase maturity mismatch for stake");
    }

    // Extract stake public key ID and verify block signature
    {
        txnouttype whichType;
        std::vector<std::vector<unsigned char>> vSolutions;
        CKeyID key_id;
        const auto &spk = txinPrevRef->vout[prevout.n].scriptPubKey;

        if (!Solver(spk, whichType, vSolutions)) {
            return state.DoS(100, false, REJECT_MALFORMED, "bad-pos-input",
                             false, "invalid Stake Input script");
        }

        if (whichType == TX_PUBKEYHASH) // pay to address type
        {
            key_id = CKeyID(uint160(vSolutions[0]));
        }
        else if (whichType == TX_PUBKEY) // pay to public key
        {
            key_id = CPubKey(vSolutions[0]).GetID();
        }
        else
        {
            return state.DoS(100, false, REJECT_MALFORMED, "bad-pos-input",
                             false, "unsupported Stake Input script");
        }

        if (!header.CheckBlockSignature(key_id)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-blk-sig",
                             false, "invalid block signature");
        }
    }

    unsigned int nInterval = 0;
    CBlockHeader rwheader = header; // const_cast could be used, but just safety
    
    bool is_valid = CheckStakeKernelHash(
            rwheader,
            *pindex_prev,
            *pindex_tx,
            *txinPrevRef,
            prevout,
            nInterval,
            true,
            fDebug);

    if (!is_valid) {
        return state.DoS(100, false, REJECT_INVALID, "bad-pos-proof");
    }

    return true;
}
