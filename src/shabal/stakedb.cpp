//stakedb.cpp
#include "shabal/linux_list.h"
#include "shabal/bpool.h"
#include "shabal/stakedb.h"
#include "blockstorage/blockstorage.h"
#include "script/standard.h"
#include "primitives/transaction.h"
#include "chainparams.h"
#include "chain.h"
#include "coins.h"
#include "dstencode.h"
#include "main.h"
#include "validation/validation.h"

#define STAKE_FMAXSIZE (8*1024*1024)
#define STAKE_BUCKET 1024
#define SAVED_ALIGN 100

typedef struct {
	int 			version;
	int 			sync_height;
	uint256 		sync_block_hash; //last block hash
} stake_head_t;

typedef struct {
	char 				addr[64];  //force len=64.   end with '\0'
	int64_t  			val; //hnode: stake amt； lnode: block height
	union {
		struct hlist_node 	hnode;
		struct list_head 	lnode;
	};
} stake_addr_t;
#define STAKE_ADDR_ITEM_LEN (sizeof(stake_addr_t) - sizeof(struct hlist_node))

typedef struct {
	char 				fname[1024];
	stake_head_t 		shead;
	int 				added_height;
	uint256 			added_block_hash;
	struct hlist_head 	hhead[STAKE_BUCKET];   //all sync block
	struct hlist_head   inchead[2][STAKE_BUCKET]; //increase block.
	struct list_head    miner_list; 				//keep block count : 1800+SAVED_ALIGN
	int 				len_miner_list; 		
	bpool_t 	 		bp; 						 //bpool
} stake_t;

static char *s_stake_buf;
static stake_t s_stake;

static void stakedb_cleanup ();
static int load_buf_to_list (const char *buf, long fsize);
static long save_list_to_buf (char *buf, long bsize);
static stake_addr_t *stake_find_addr (struct hlist_head *head, const char *addr);
static CBlockIndex *find_next_block_idx (const CBlockIndex *pBlockIndex, const uint256 now_hash);
static int stakedb_step_to (CBlockIndex *pBlockIndex);



static void stakedb_cleanup () {
	if (s_stake.fname[0]) {
		bpool_cleanup (&s_stake.bp);

		memset (&s_stake, 0, sizeof(s_stake));
	}
}

static int load_buf_to_list (const char *buf, long fsize) {
	long off = sizeof(stake_head_t);
	stake_head_t *shead = (stake_head_t*)buf;
	stake_addr_t *saddr;

	if (shead->version != 1) {
		LOGAF("version error...must 1");
		return -1;
	}
	memcpy (&s_stake.shead, buf, sizeof(stake_head_t));
	s_stake.added_height = s_stake.shead.sync_height;
	s_stake.added_block_hash = s_stake.shead.sync_block_hash;

	stake_addr_t *paddr = (stake_addr_t*)(buf + sizeof(stake_head_t));

	for (; off + STAKE_ADDR_ITEM_LEN <= fsize; off += STAKE_ADDR_ITEM_LEN) {
		paddr = (stake_addr_t*)(buf + off);
		saddr = (stake_addr_t*)bpool_calloc_block (&s_stake.bp);
		memcpy (saddr, buf+off, STAKE_ADDR_ITEM_LEN);
		//Do not think there will be duplicate keys
		unsigned idx = hlist_str_hash (saddr->addr, STAKE_BUCKET);
		hlist_add_head (&saddr->hnode, &s_stake.hhead[idx]);
	}
	return 0;
}

static long save_list_to_buf (char *buf, long bsize) {
	unsigned incidx, idx;
	long off = sizeof(stake_head_t);
	if (s_stake.added_height > chainActive.Height() + SAVED_ALIGN) {
		LOGAF("logic failed. %d > %d + 100", s_stake.added_height, chainActive.Height());
		return 0;
	}

	s_stake.shead.sync_height = s_stake.added_height - SAVED_ALIGN;
	CBlockIndex *pblockindex = chainActive[s_stake.shead.sync_height];
	s_stake.shead.sync_block_hash = pblockindex->GetBlockHeader().GetHash();
	memcpy (buf, &s_stake.shead, sizeof(stake_head_t));

	incidx = (s_stake.added_height / SAVED_ALIGN) % 2; //from the old list

	for (idx = 0; idx < sizeof(s_stake.hhead)/sizeof(s_stake.hhead[0]); idx++) {
		struct hlist_head *pinchead = &s_stake.inchead[incidx][idx];

		stake_addr_t *op = NULL, *op2;
		hlist_node *tmpnode;
		hlist_for_each_entry_safe (op, tmpnode, &s_stake.hhead[idx], hnode) {
			op2 = stake_find_addr (pinchead, op->addr);
			if (op2) {
				LOGAF("synch=%d to inc %s %lld --> %lld", s_stake.shead.sync_height, op->addr, op->val, op->val + op2->val);
				op->val += op2->val;
				hlist_del (&op2->hnode);
				bpool_free_block (&s_stake.bp, op2);
			}

			if (op->val > 0 && off + STAKE_ADDR_ITEM_LEN <= bsize) {
				memcpy (buf+off, op, STAKE_ADDR_ITEM_LEN);
				off += STAKE_ADDR_ITEM_LEN;
				LOGAF("addoff -->%ld", off);
			}
			if (op->val <= 0) {
				LOGAF("synch=%d to del %s", s_stake.shead.sync_height, op->addr);
				hlist_del (&op->hnode);
				bpool_free_block (&s_stake.bp, op);
			} 
		}
		//Collect and transfer new additions
		hlist_for_each_entry_safe (op, tmpnode, pinchead, hnode) {
			hlist_del (&op->hnode);
			if (op->val != 0) {
				hlist_add_head (&op->hnode, &s_stake.hhead[idx]);
				LOGAF("synch=%d to move %s %lld", s_stake.shead.sync_height, op->addr, op->val);
			}
		}
	}
	LOGAF("Succeed to save stakedb. h=%d size=%ld. incidx=%d", s_stake.shead.sync_height, off, incidx);
	return off;
}

static stake_addr_t *stake_find_addr (struct hlist_head *head, const char *addr) {
	stake_addr_t *op = NULL;
	hlist_for_each_entry (op, head, hnode) {
		if (strcmp (op->addr, addr) == 0) {
			return op;
		}
	}
	return NULL;
}

static CBlockIndex *find_next_block_idx (CBlockIndex *pBlockIndex, const uint256 now_hash) {
	CBlockIndex *pnext = pBlockIndex;
	if (pnext && pnext->GetBlockHeader().GetHash() == now_hash) {
		return pnext; //not the next
	}

	while (pnext && pnext->pprev && pnext->pprev->GetBlockHeader().GetHash() != now_hash) {
		pnext = pnext->pprev;
	}

	if (!pnext || !pnext->pprev) {
		return NULL;
	}
	return pnext;
}


//--------------------------------------------------------------------------------------------------

//return >=0 height. <0 error
int stakedb_load (const char *fname) {
	int i = 0;
	long fsize = 0;
	ssize_t ret = 0;

	memset (&s_stake, 0, sizeof(s_stake));
	strncpy (s_stake.fname, fname, sizeof(s_stake.fname)-1);
	for (i=0; i<STAKE_BUCKET; i++) {
		INIT_HLIST_HEAD(&s_stake.hhead[i]);
		INIT_HLIST_HEAD(&s_stake.inchead[0][i]);
		INIT_HLIST_HEAD(&s_stake.inchead[1][i]);
	}
	INIT_LIST_HEAD(&s_stake.miner_list);
	bpool_init (&s_stake.bp, SIZE_AUTO_EXPAND, sizeof(stake_addr_t));

	if (s_stake_buf == NULL) {
		s_stake_buf = (char*)malloc(STAKE_FMAXSIZE);
		if (s_stake_buf == NULL) {
			LOGAF("Failed to alloc buf. size=%d", STAKE_FMAXSIZE);
			return -1;
		}
	}

	fsize = read_filesize (fname);
	if (fsize <= (long)sizeof(stake_head_t) || fsize > STAKE_FMAXSIZE) {
		s_stake.shead.version = 1;
		s_stake.shead.sync_height = 0;
		s_stake.shead.sync_block_hash = Params().GenesisBlock().GetHash();
		write_bin_file (fname, (const char*)&s_stake.shead, sizeof(s_stake.shead));
		LOGAF("init stakedb. fsize=%d", fsize);
	} else {
		if (read_bin_file (fname, s_stake_buf, fsize) != fsize) {
			LOGAF("Failed to read stakedb. fsize=%d", fsize);
			goto _Failed;
		}

		if (load_buf_to_list (s_stake_buf, fsize) == -1) {
			LOGAF("Failed to load stakedb. fsize=%d", fsize);
			goto _Failed;
		}
		LOGA("Succeed to load stakedb. height=%d", s_stake.shead.sync_height);
	}

	return 0;

_Failed:

	stakedb_cleanup ();
	return -1;
}

static int stakedb_step_to (CBlockIndex *pBlockIndex) {
	unsigned incidx;
	struct hlist_head *head;
	struct hlist_head *pinchead;
	stake_addr_t *r;
	std::string addrstr;
	const char *addr = NULL;
	CAmount amt;
	CBlock block;
	const Consensus::Params &consensusParams = Params().GetConsensus();

	if (pBlockIndex->nHeight != s_stake.added_height + 1) {
		LOGAF("Failed to step_to %d. now is %d", pBlockIndex->nHeight, s_stake.added_height);
		return -1;
	}

	if (pBlockIndex->pprev && pBlockIndex->pprev->GetBlockHash() != s_stake.added_block_hash) {
		LOGAF("Failed to step_to %d. hash not equ.", pBlockIndex->nHeight);
		return -1;
	}
	//find out all : coinbase stakein, unstake
	if (!ReadBlockFromDisk (block, pBlockIndex, consensusParams)) {
		LOGAF("Failed to step to %d. read block failed.", pBlockIndex->nHeight);
		return -1;
	}
	incidx = (pBlockIndex->nHeight / SAVED_ALIGN) % 2;

	for (const auto &ptx : block.vtx) {
		if (ptx->IsCoinBase()) {
			r = (stake_addr_t*)bpool_calloc_block (&s_stake.bp);
			addrstr = EncodeScriptPubKey (ptx->vout[1].scriptPubKey);
			strncpy (r->addr, addrstr.c_str(), sizeof(r->addr)-1);
			r->val = pBlockIndex->nHeight;
			list_add_tail (&r->lnode, &s_stake.miner_list);
			LOGAF("found h=%d coinbase %s, len=%d", pBlockIndex->nHeight, addrstr.c_str(), s_stake.len_miner_list);
			//remove first
			if (s_stake.len_miner_list >= uPeriod + SAVED_ALIGN) {
				r = (stake_addr_t*)list_first_entry(&s_stake.miner_list, stake_addr_t, lnode);
				list_del (&r->lnode);
				bpool_free_block (&s_stake.bp, r);
			} else {
				s_stake.len_miner_list++;
			}
			continue;
		}
		int iin, iout;
		auto ptype = ptx->GetPledgeType(iin, iout);
		if (ptype == DCOP_NONE) {
			continue;
		}
		//get addr
		addrstr = EncodeScriptPubKey (ptx->vout[iout].scriptPubKey);
		if (!IsValidDestinationString (addrstr)) {
			LOGAF("not valid addr ? %s", addrstr);
			continue;
		}
		addr = addrstr.c_str();
		
		if (ptype == DCOP_PLEDGE) {
			amt = ptx->vout[iin].nValue;
		} else {
			const COutPoint &cop = ptx->vin[0].prevout;
			CTransactionRef ptx2;
			uint256 hashBlockIn;
			if (!GetTransaction (cop.hash, ptx2, consensusParams, hashBlockIn, true, nullptr)) {
				LOGAF("logic failed....");
				return -1;
			}
			if (ptx2->GetPledgeType(iin, iout) != DCOP_PLEDGE) {
				LOGAF("logic failed. not pledgeto");
				return -1;
			}
			amt = - ptx2->vout[iin].nValue;

			// LOGAF("==== %lld, %lld", amt, ptx2->vout[iin].nValue);
		}

		pinchead = &s_stake.inchead[incidx][hlist_str_hash(addr, STAKE_BUCKET)];
		r = stake_find_addr (pinchead, addr);
		if (r == NULL) {
			r = (stake_addr_t*)bpool_calloc_block (&s_stake.bp);
			strncpy (r->addr, addr, sizeof(r->addr)-1);
			hlist_add_head (&r->hnode, pinchead);
		}

		LOGAF("Found %sstake in block %d. %s. origin=%lld, inc=%lld", ((ptype==DCOP_PLEDGE)?"":"un"), pBlockIndex->nHeight, addr, r->val, amt);

		r->val += amt;
	}

	LOGAF("added %d", pBlockIndex->nHeight);
	s_stake.added_height = pBlockIndex->nHeight;
	s_stake.added_block_hash = pBlockIndex->GetBlockHeader().GetHash();

	if (s_stake.added_height % SAVED_ALIGN == 0) {
		//sync to file.
		long fsize = save_list_to_buf (s_stake_buf, STAKE_FMAXSIZE);
		write_bin_file (s_stake.fname, s_stake_buf, fsize);
	}
	return 0;
}

int stakedb_restep_to (CBlockIndex *pBlockIndex) {
	unsigned i;
	uint256 now_hash;
	if (pBlockIndex->nHeight < s_stake.shead.sync_height) {
		return -1; //cancel
	}

	CBlockIndex *pnext = find_next_block_idx (pBlockIndex, s_stake.added_block_hash);
	if (pnext) {
		if (pnext == pBlockIndex && s_stake.added_block_hash == pBlockIndex->GetBlockHeader().GetHash()) {
			LOG(COINDB, "return: %d", s_stake.added_height);
			return s_stake.added_height;
		}
		now_hash = s_stake.added_block_hash;
		LOG(COINDB, "found next height: %d", pnext->nHeight);
	} else {
		LOG(COINDB, "not found next height, reto %d", s_stake.shead.sync_height);
		stake_addr_t *op, *op2;
		hlist_node *tmp_hnode;
		//Drop the incremental part
		now_hash = s_stake.shead.sync_block_hash;

		s_stake.added_height = s_stake.shead.sync_height;
		s_stake.added_block_hash = s_stake.shead.sync_block_hash;

		for (i=0; i<STAKE_BUCKET; i++) {

			hlist_for_each_entry_safe (op, tmp_hnode, &s_stake.inchead[0][i], hnode) {
				hlist_del (&op->hnode);
				bpool_free_block (&s_stake.bp, op);
			}
			hlist_for_each_entry_safe (op, tmp_hnode, &s_stake.inchead[1][i], hnode) {
				hlist_del (&op->hnode);
				bpool_free_block (&s_stake.bp, op);
			}
		}

		list_for_each_entry_safe_reverse (op, op2, &s_stake.miner_list, lnode) {
			if (op->val <= s_stake.shead.sync_height) {
				break;
			}
			list_del (&op->lnode);
			bpool_free_block (&s_stake.bp, op);
			s_stake.len_miner_list--;
		}
	}

	for (; (pnext = find_next_block_idx(pBlockIndex, now_hash)) != NULL; 
		now_hash = pnext->GetBlockHeader().GetHash()) 
	{
		if ( pnext == pBlockIndex && now_hash == pBlockIndex->GetBlockHeader().GetHash()) {
			LOG(COINDB, "skip break");
			break;
		}

		if (stakedb_step_to (pnext) != 0) {
			LOGAF("Failed to stepto next. %d --> --> %d", pnext->nHeight, pBlockIndex->nHeight);
			return -1;
		}
	}
	return s_stake.added_height;
}

uint64_t stakedb_get_stake (const char* addr) {
	unsigned idx = hlist_str_hash(addr, STAKE_BUCKET);
	stake_addr_t *op = stake_find_addr (&s_stake.hhead[idx], addr);
	stake_addr_t *op2 = stake_find_addr (&s_stake.inchead[0][idx], addr);
	stake_addr_t *op3 = stake_find_addr (&s_stake.inchead[1][idx], addr);
	return (op ? op->val : 0) + (op2 ? op2->val : 0) + (op3 ? op3->val : 0);
}

int stakedb_get_mined (const char *addr)
{
	int nblock = 0;
	int npre = 0;
	stake_addr_t *r;
	while (s_stake.len_miner_list < uPeriod) {
		r = (stake_addr_t*)list_first_entry (&s_stake.miner_list, stake_addr_t, lnode);
		if (!r) {
			LOGAF("logic failed.");
			return -1;
		}
		if (r->val <= 1) {
			break;
		}
		if (r->val > chainActive.Height()) {
			LOGAF("need height: %ld > %ld", r->val, chainActive.Height());
			return -1;
		}
		CBlockIndex *pBlockIndex = chainActive[r->val - 1];
		if (pBlockIndex->nHeight == 0) {
			break;
		}
		CBlock block;
		const Consensus::Params &consensusParams = Params().GetConsensus();
		if (!ReadBlockFromDisk (block, pBlockIndex, consensusParams)) {
			LOGAF("not found disk block %d", pBlockIndex->nHeight);
			return -1;
		}
		const auto &ptx = block.vtx[0];
		if (!ptx->IsCoinBase() || ptx->vout.size() != 2){
			LOGAF("not found coin base. height=%d, nvout=%d", pBlockIndex->nHeight, ptx->vout.size());
			return -1;
		}

		r = (stake_addr_t*)bpool_calloc_block (&s_stake.bp);
		std::string addrstr = EncodeScriptPubKey (ptx->vout[1].scriptPubKey);
		strncpy (r->addr, addrstr.c_str(), sizeof(r->addr)-1);
		r->val = pBlockIndex->nHeight;
		list_add_first (&r->lnode, &s_stake.miner_list);
		LOGAF("found 2 h=%d coinbase %s, len=%d", pBlockIndex->nHeight, addrstr.c_str(), s_stake.len_miner_list);
		s_stake.len_miner_list++;
	}
	list_for_each_entry_reverse (r, &s_stake.miner_list, lnode) {
		// if (r->val > height) {
		// 	continue;
		// }

		if (strcmp (addr, r->addr) == 0) {
			nblock++;
		}

		if (++npre >= uPeriod - 1) { //1799
			break;
		}
	}
	return nblock;
}

int stakedb_get_height () {
	if (s_stake.fname[0]) {
		return s_stake.added_height;
	}
	return -1;
}

void stakedb_debug_print(const char *addr) {
	int nblock = 0;
	int npre = 0;
	int print = 0;

	stake_addr_t *r = (stake_addr_t*)list_first_entry(&s_stake.miner_list, stake_addr_t, lnode);
	if (!r) {
		LOGA("stakedb_debug no miner_list.");
		return;
	}
	list_for_each_entry_reverse (r, &s_stake.miner_list, lnode) {
		print = 0;
		if (addr && strcmp(addr, r->addr) == 0) {
			print = 1;
			nblock ++;
		} else if (!addr || addr[0]=='\0') {
			print = 1;
		}
		if (print) {
			LOG(COINDB, "stakedb_debug %s : %d[%d]", r->addr, r->val, nblock);
		}
		if (++npre >= uPeriod - 1) { //1799
			break;
		}
	}
	LOGA( "print %s sync=%d --> %d End PRINT stakedb_debug_print", addr?addr:"NULL", s_stake.shead.sync_height, s_stake.added_height);
}

int stakedb_reinit () {
	char fname[sizeof(s_stake.fname)] = {0};
	strncpy (fname, s_stake.fname, sizeof(fname)-1);

	LOGAF("???");
	
	write_bin_file (fname, (const char*)&s_stake.shead, sizeof(s_stake.shead));

	stakedb_cleanup();

	stakedb_load (fname);
}