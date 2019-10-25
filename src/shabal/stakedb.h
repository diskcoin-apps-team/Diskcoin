//stakedb.h
#pragma once

#include "chain.h"

int stakedb_load (const char *fname);

int stakedb_reinit ();

int stakedb_restep_to (CBlockIndex *pBlockIndex);

uint64_t stakedb_get_stake (const char* addr);

int stakedb_get_mined (const char *addr);

int stakedb_get_height ();

void stakedb_debug_print(const char *addr);
