// Copyright (c) 2019-2019 The Energi Core developers
// Distributed under the MIT software license, see the accompanying
// Copyright (c) 2012-2013 The PPCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KERNEL_H
#define BITCOIN_KERNEL_H

#include "amount.h"
#include "streams.h"

class CBlockIndex;
class CBlockHeader;
class CTransaction;
class COutPoint;
class CValidationState;
class uint256;

namespace Consensus {
    class Params;
}

static constexpr CAmount MIN_STAKE_AMOUNT = COIN;
static constexpr int64_t MAX_POS_BLOCK_AHEAD_TIME = 180;
static constexpr int64_t MAX_POS_BLOCK_AHEAD_SAFETY_MARGIN = 5;

// Compute the hash modifier for proof-of-stake
bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier);

// Check whether stake kernel meets hash target
// Sets hashProofOfStake on success return
uint256 stakeHash(unsigned int nTimeTx, CDataStream ss, unsigned int prevoutIndex, uint256 prevoutHash, unsigned int nTimeBlockFrom);
bool CheckStakeKernelHash(unsigned int nBits, const CBlockIndex &blockFrom, const CTransaction txPrev, const COutPoint prevout, unsigned int& nTimeTx, unsigned int nHashDrift, bool fCheck, uint256& hashProofOfStake, uint64_t &nStakeModifier, bool fPrintProofOfStake = false);

// Check kernel hash target and coinstake signature
// Sets hashProofOfStake on success return
bool CheckProofOfStake(CValidationState &state, const CBlockHeader &block, const Consensus::Params& consensus);

#endif // BITCOIN_KERNEL_H
