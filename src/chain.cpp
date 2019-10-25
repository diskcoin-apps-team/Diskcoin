// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2015-2018 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"
#include "main.h"

// diskcoin
#include <string.h>
#include "shabal/sph_shabal.h"

using namespace std;

void CalculateSignature(const CBlockIndex *pindex, unsigned char* sig)
{
	unsigned char data[32+8+sizeof(uint256)+1] = {};
    
    uint64_t plotter_id = pindex->GetBlockHeader().nPlotterId;

    memcpy(data, pindex->GetBlockHeader().sig, 32);
    memcpy(data+32, &plotter_id, sizeof(plotter_id));
    memcpy(data+40, pindex->phashBlock, sizeof(uint256));

    sph_shabal_context ctx;
    sph_shabal256_init(&ctx);
    
    sph_shabal256(&ctx, (unsigned char *)data, sizeof(data)-1);
    sph_shabal256_close(&ctx, sig);
}

uint64_t CalculateBaseTarget(const CBlockIndex *pindex)
{
    //--->fast test--->
    if (Params().NetworkIDString() == "regtest") {
        return  Params().MaxBaseTarget();    
    }
    //<--
	uint64_t base_target = 0;

    const CBlockIndex *cur_block = pindex;
	int cur_height = pindex->nHeight;

	auto MaxBaseTarget = Params().MaxBaseTarget();
	auto nPowTargetSpacing = Params().GetConsensus().nPowTargetSpacing;

	if (cur_height < 4)
    {
        return  Params().MaxBaseTarget();
    }
    
    uint32_t updownPercent = 0;
    uint64_t cur_base_target = 0;
    uint64_t new_base_target = 0;

    if (cur_height < 2700)
    {
        uint64_t avg_base_target = 0;
        const CBlockIndex *it_block = cur_block;
        do
        {   
            avg_base_target += it_block->GetBlockHeader().nBaseTarget;
            it_block = it_block->pprev;
        }while(it_block->nHeight>cur_height-4);

        uint32_t dif_time = cur_block->nTime - it_block->nTime;

        cur_base_target = avg_base_target / 4;
        new_base_target = cur_base_target * dif_time / (4 * nPowTargetSpacing);

        updownPercent = 10;
    }
    else
    {
        const CBlockIndex *it_block = cur_block->pprev;
        uint64_t avg_base_target = it_block->GetBlockHeader().nBaseTarget;
        int block_counter = 1;
        do
        {
            it_block = it_block->pprev;
            block_counter++;
            avg_base_target = (avg_base_target*block_counter + 
                it_block->GetBlockHeader().nBaseTarget) / (block_counter+1);
        }while(block_counter < 24);

        uint32_t dif_time = cur_block->nTime - it_block->nTime;
        uint32_t target_time_span = 24 * nPowTargetSpacing;

        if (dif_time < target_time_span/2)
        {
            dif_time = target_time_span / 2;
        }
        
        if (dif_time > target_time_span*2)
        {
            dif_time = target_time_span * 2;
        }

        cur_base_target = cur_block->pprev->GetBlockHeader().nBaseTarget;
        new_base_target = avg_base_target * dif_time / target_time_span;

        updownPercent = 20;
    }

    if (new_base_target==0 || new_base_target > MaxBaseTarget)
    {
        new_base_target = MaxBaseTarget;
    }
    if (new_base_target<(cur_base_target*(100-updownPercent)/100))
    {
        new_base_target = cur_base_target * (100-updownPercent) / 100;
    }
    if (0 == new_base_target)
    {
        new_base_target = 1;
    }

    uint64_t two_fold_cur_base_target = cur_base_target * (100+updownPercent) / 100;
    if (two_fold_cur_base_target == 0)
    {
        two_fold_cur_base_target = MaxBaseTarget;
    }
    if (new_base_target>two_fold_cur_base_target)
    {
        new_base_target = two_fold_cur_base_target; 
    }

	return new_base_target;
}

uint64_t CChain::GetBaseTarget()
{
    static int height = 0;
    static uint64_t base_target = 0;

    int cur_height = this->Height();
    if (cur_height == height && 0 != base_target)
    {
        return base_target;
    }

    const CBlockIndex *cur_block = (*this)[cur_height];

    base_target = CalculateBaseTarget(cur_block);
    
    height = cur_height;

    return base_target;
}

const unsigned char* CChain::GetGenerationSignature()
{
    static int height = -1;
    static unsigned char gensig[32] = {};

    int64_t cur_height = this->Height();
    if (cur_height == height)
    {
        return gensig;
    }

	const CBlockIndex *cur_block = (*this)[cur_height];

    CalculateSignature(cur_block, gensig);

	height = cur_height;
    
    return gensig;
}

/**
 * CChain implementation
 */
void CChain::SetTip(CBlockIndex *pindex)
{
    if (pindex == nullptr)
    {
        vChain.clear();
        tip = nullptr;
        return;
    }
    vChain.resize(pindex->nHeight + 1);
    tip = pindex;
    while (pindex && vChain[pindex->nHeight] != pindex)
    {
        vChain[pindex->nHeight] = pindex;
        pindex = pindex->pprev;
    }
}

CBlockLocator CChain::GetLocator(const CBlockIndex *pindex) const
{
    int nStep = 1;
    std::vector<uint256> vHave;
    vHave.reserve(32);

    if (!pindex)
        pindex = Tip();
    while (pindex)
    {
        vHave.push_back(pindex->GetBlockHash());
        // Stop when we have added the genesis block.
        if (pindex->nHeight == 0)
            break;
        // Exponentially larger steps back, plus the genesis block.
        int nHeight = std::max(pindex->nHeight - nStep, 0);
        if (Contains(pindex))
        {
            // Use O(1) CChain index if possible.
            pindex = (*this)[nHeight];
        }
        else
        {
            // Otherwise, use O(log n) skiplist.
            pindex = pindex->GetAncestor(nHeight);
        }
        if (vHave.size() > 10)
            nStep *= 2;
    }

    return CBlockLocator(vHave);
}

const CBlockIndex *CChain::FindFork(const CBlockIndex *pindex) const
{
    if (pindex == NULL)
    {
        return NULL;
    }
    if (pindex->nHeight > Height())
        pindex = pindex->GetAncestor(Height());
    while (pindex && !Contains(pindex))
        pindex = pindex->pprev;
    return pindex;
}

/** Turn the lowest '1' bit in the binary representation of a number into a '0'. */
int static inline InvertLowestOne(int n) { return n & (n - 1); }
/** Compute what height to jump back to with the CBlockIndex::pskip pointer. */
int static inline GetSkipHeight(int height)
{
    if (height < 2)
        return 0;

    // Determine which height to jump back to. Any number strictly lower than height is acceptable,
    // but the following expression seems to perform well in simulations (max 110 steps to go back
    // up to 2**18 blocks).
    return (height & 1) ? InvertLowestOne(InvertLowestOne(height - 1)) + 1 : InvertLowestOne(height);
}

CBlockIndex *CBlockIndex::GetAncestor(int height)
{
    if (height > nHeight || height < 0)
        return nullptr;

    CBlockIndex *pindexWalk = this;
    int heightWalk = nHeight;
    while (heightWalk > height)
    {
        int heightSkip = GetSkipHeight(heightWalk);
        int heightSkipPrev = GetSkipHeight(heightWalk - 1);
        if (pindexWalk->pskip != nullptr &&
            (heightSkip == height ||
                (heightSkip > height && !(heightSkipPrev < heightSkip - 2 && heightSkipPrev >= height))))
        {
            // Only follow pskip if pprev->pskip isn't better than pskip->pprev.
            pindexWalk = pindexWalk->pskip;
            heightWalk = heightSkip;
        }
        else
        {
            assert(pindexWalk->pprev);
            pindexWalk = pindexWalk->pprev;
            heightWalk--;
        }
    }
    return pindexWalk;
}

const CBlockIndex *CBlockIndex::GetAncestor(int height) const
{
    return const_cast<CBlockIndex *>(this)->GetAncestor(height);
}

void CBlockIndex::BuildSkip()
{
    if (pprev)
        pskip = pprev->GetAncestor(GetSkipHeight(nHeight));
}

/** Member helper functions needed to implement time based fork activation
 *
 * In the following comments x-1 is used to identify the first block for which GetMedianTimePast()
 * (GMTP) is equal or greater than fork time
 * Instead we use 'x' to indicate the first block mined after the time based trigger fired.
 * A fork is considered to be enabled at height x-1 and activated at height x.
 * We chose this naming scheme because usually block 'x' has to satisfy additional conditions
 * on e.g. block size in the UAHF (Aug, 1st 2017 protocol upgrade) case.
 *
 * The following helper will check if a given block belongs to 4 different intervals, namely:
 *
 * - forkActivated: [x,+inf)
 * - forkActivateNow: [x,x]
 * - forkActiveOnNextBlock: [x-1,+inf)
 * - forkAtNextBlock: [x-1,x-1]
 */

/** return true for every block from fork block and forward [x,+inf)
 * state: fork activated */
bool CBlockIndex::forkActivated(int time) const
{
    if (time == 0)
        return false;

    if (pprev && pprev->GetMedianTimePast() >= time)
    {
        return true;
    }
    return false;
}

/** return true only if we are exactly on the fork block [x,x]
 * state: fork activated */
bool CBlockIndex::forkActivateNow(int time) const
{
    if (time == 0)
        return false;
    return (pprev && pprev->forkAtNextBlock(time));
}

/** This will check if the Fork will be enabled at the next block
 * i.e. we are at block x - 1, [x-1, +inf]
 * state fork: enabled or activated */
bool CBlockIndex::IsforkActiveOnNextBlock(int time) const
{
    if (time == 0)
        return false;
    // if the fork is already activated
    if (forkActivated(time))
        return true;
    if (GetMedianTimePast() >= time)
        return true;
    return false;
}

/* return true only if 1st condition is true (Median past time >= fork time)
 * and not the 2nd, i.e. we are at precisely [x-1,x-1]
 * state: fork enabled but not activated */
bool CBlockIndex::forkAtNextBlock(int time) const
{
    if (time == 0)
        return false;

    // if the fork is already activated
    if (forkActivated(time))
        return false;

    if (GetMedianTimePast() >= time)
        return true;
    return false;
}

std::string CBlockFileInfo::ToString() const
{
    return strprintf("CBlockFileInfo(blocks=%u, size=%u, heights=%u...%u, time=%s...%s)", nBlocks, nSize, nHeightFirst,
        nHeightLast, DateTimeStrFormat("%Y-%m-%d", nTimeFirst), DateTimeStrFormat("%Y-%m-%d", nTimeLast));
}
