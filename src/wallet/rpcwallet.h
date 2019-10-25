// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_RPCWALLET_H
#define BITCOIN_WALLET_RPCWALLET_H

class CRPCTable;

void RegisterWalletRPCCommands(CRPCTable &tableRPC);
UniValue listminedblock(const UniValue &params, bool fHelp);
UniValue liststakein(const UniValue &params, bool fHelp);
UniValue liststakeout(const UniValue &params, bool fHelp);
UniValue listunstake(const UniValue &params, bool fHelp);
UniValue getaddrinfo(const UniValue &params, bool fHelp);

#endif // BITCOIN_WALLET_RPCWALLET_H
