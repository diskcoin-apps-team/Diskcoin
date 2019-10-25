// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include "consensus/params.h"

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;
class arith_uint256;

unsigned int GetNextWorkRequired(const CBlockIndex *pindexLast, const CBlockHeader *pblock, const Consensus::Params &);
unsigned int CalculateNextWorkRequired(const CBlockIndex *pindexLast,
    int64_t nFirstBlockTime,
    const Consensus::Params &);

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params &);
arith_uint256 GetBlockProof(const CBlockIndex &block);

/** Return the time it would take to redo the work difference between from and to, assuming the current hashrate
 * corresponds to the difficulty at tip, in seconds. */
int64_t GetBlockProofEquivalentTime(const CBlockIndex &to,
    const CBlockIndex &from,
    const CBlockIndex &tip,
    const Consensus::Params &);

/**
 * Bitcoin cash's difficulty adjustment mechanism.
 */
uint32_t GetNextCashWorkRequired(const CBlockIndex *pindexPrev,
    const CBlockHeader *pblock,
    const Consensus::Params &params);

// diskcoin
void CalculateSignature(const CBlockIndex *pindex, unsigned char* sig);
uint64_t CalculateBaseTarget(const CBlockIndex *pindex);
uint64_t CalculateBest(int height, const unsigned char* gen_sig, uint64_t plotter_id, uint64_t nonce);

bool CheckProofOfCapacity(uint256 hash, const Consensus::Params &params);
bool CheckHeaderProofOfCapacity(const CBlockHeader &header, const Consensus::Params &params);


#endif // BITCOIN_POW_H
