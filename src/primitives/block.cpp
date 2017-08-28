// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"
#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "crypto/common.h"

uint256 CBlockHeader::GetHash(uint32_t blockHeight) const
{
    // BEGIN(a) = ((char*)&(a))
    // END(a)   = ((char*)&((&(a))[1]))
    auto egihash_result = egihash::full::hash(egihash::dag_t(blockHeight/egihash::constants::EPOCH_LENGTH),
                                egihash::h256_t(&nVersion, sizeof(nVersion)), // TODO: this is not right, should be hash of serialized block header without mixhash + nonce
                                nNonce);
    std::vector<uint8_t> val(32, 0);
    std::memcpy(&val[0], &(egihash_result.value.b[0]), 32);
    return uint256(val);
}

std::string CBlock::ToString(uint32_t blockHeight) const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, hashMix=%s, vtx=%u)\n",
        GetHash(blockHeight).ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        hashMix.ToString(),
        vtx.size());
    for (unsigned int i = 0; i < vtx.size(); i++)
    {
        s << "  " << vtx[i].ToString() << "\n";
    }
    return s.str();
}
