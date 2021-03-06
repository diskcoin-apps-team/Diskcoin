// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2018 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sigcache.h"

#include "memusage.h"
#include "pubkey.h"
#include "random.h"
#include "uint256.h"
#include "util.h"

#include "cuckoocache.h"
#include <boost/thread.hpp>

// std::shared_mutex not available until c++17, should upgrade when possible
// std::shared_lock not available until c++14,  should upgrade when possible
//#include <shared_mutex>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>


namespace
{
/**
 * We're hashing a nonce into the entries themselves, so we don't need extra
 * blinding in the set hash computation.
 *
 * This may exhibit platform endian dependent behavior but because these are
 * nonced hashes (random) and this state is only ever used locally it is safe.
 * All that matters is local consistency.
 */
class SignatureCacheHasher
{
public:
    template <uint8_t hash_select>
    uint32_t operator()(const uint256 &key) const
    {
        static_assert(hash_select < 8, "SignatureCacheHasher only has 8 hashes available.");
        uint32_t u;
        std::memcpy(&u, key.begin() + 4 * hash_select, 4);
        return u;
    }
};

/**
 * Declare which flags absolutely do not affect VerifySignature() result.
 * We this to reduce unnecessary cache misses (such as when policy and consensus
 * flags differ on unrelated aspects).
 */
static const uint32_t INVARIANT_FLAGS =
    SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC | SCRIPT_VERIFY_DERSIG | SCRIPT_VERIFY_LOW_S |
    SCRIPT_VERIFY_NULLDUMMY | SCRIPT_VERIFY_SIGPUSHONLY | SCRIPT_VERIFY_MINIMALDATA |
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS | SCRIPT_VERIFY_CLEANSTACK | SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY |
    SCRIPT_VERIFY_CHECKSEQUENCEVERIFY | SCRIPT_VERIFY_MINIMALIF | SCRIPT_VERIFY_NULLFAIL |
    SCRIPT_VERIFY_COMPRESSED_PUBKEYTYPE | SCRIPT_ENABLE_SIGHASH_FORKID | SCRIPT_ENABLE_REPLAY_PROTECTION |
    SCRIPT_ENABLE_CHECKDATASIG | SCRIPT_ALLOW_SEGWIT_RECOVERY;

/**
 * Valid signature cache, to avoid doing expensive ECDSA signature checking
 * twice for every transaction (once when accepted into memory pool, and
 * again when accepted into the block chain)
 */
class CSignatureCache
{
private:
    //! Entries are SHA256(nonce || flags || signature hash || public key ||
    //! signature):
    uint256 nonce;
    typedef CuckooCache::cache<uint256, SignatureCacheHasher> map_type;
    map_type setValid;
    boost::shared_mutex cs_sigcache;

public:
    CSignatureCache() { GetRandBytes(nonce.begin(), 32); }
    void ComputeEntry(uint256 &entry,
        const std::vector<uint8_t> &vchSig,
        const CPubKey &pubkey,
        const uint256 &hash,
        uint32_t flags)
    {
        flags &= ~INVARIANT_FLAGS;
        CSHA256()
            .Write(nonce.begin(), 32)
            .Write(reinterpret_cast<uint8_t *>(&flags), sizeof(flags))
            .Write(hash.begin(), 32)
            .Write(&pubkey[0], pubkey.size())
            .Write(&vchSig[0], vchSig.size())
            .Finalize(entry.begin());
    }

    bool Get(const uint256 &entry, const bool erase)
    {
        boost::shared_lock<boost::shared_mutex> lock(cs_sigcache);
        return setValid.contains(entry, erase);
    }

    void Set(uint256 &entry)
    {
        boost::unique_lock<boost::shared_mutex> lock(cs_sigcache);
        setValid.insert(entry);
    }
    uint32_t setup_bytes(size_t n) { return setValid.setup_bytes(n); }
};

/* In previous versions of this code, signatureCache was a local static variable
 * in CachingTransactionSignatureChecker::VerifySignature.  We initialize
 * signatureCache outside of VerifySignature to avoid the atomic operation per
 * call overhead associated with local static variables even though
 * signatureCache could be made local to VerifySignature.
*/
static CSignatureCache signatureCache;
}

// To be called once in AppInit2/TestingSetup to initialize the signatureCache
void InitSignatureCache()
{
    size_t nMaxCacheSize = GetArg("-maxsigcachesize", DEFAULT_MAX_SIG_CACHE_SIZE) * ((size_t)1 << 20);
    if (nMaxCacheSize <= 0)
        return;
    size_t nElems = signatureCache.setup_bytes(nMaxCacheSize);
    LOGA("Using %zu MiB out of %zu requested for signature cache, able to store %zu elements\n",
        (nElems * sizeof(uint256)) >> 20, nMaxCacheSize >> 20, nElems);
}

template <typename F>
bool RunMemoizedCheck(const std::vector<uint8_t> &vchSig,
    const CPubKey &pubkey,
    const uint256 &sighash,
    uint32_t flags,
    bool storeOrErase,
    const F &fun)
{
    uint256 entry;
    signatureCache.ComputeEntry(entry, vchSig, pubkey, sighash, flags);
    if (signatureCache.Get(entry, !storeOrErase))
    {
        return true;
    }

    if (!fun())
        return false;

    if (storeOrErase)
    {
        signatureCache.Set(entry);
    }
    return true;
}

bool CachingTransactionSignatureChecker::IsCached(const std::vector<uint8_t> &vchSig,
    const CPubKey &pubkey,
    const uint256 &sighash) const
{
    return RunMemoizedCheck(vchSig, pubkey, sighash, nFlags, true, [] { return false; });
}


#if 0 // TODO
bool CachingTransactionSignatureChecker::VerifySignature(const std::vector<unsigned char> &vchSig,
    const CPubKey &pubkey,
    const uint256 &sighash) const
{
    uint256 entry;
    signatureCache.ComputeEntry(entry, sighash, vchSig, pubkey);

    if (signatureCache.Get(entry))
    {
        if (!store)
        {
            signatureCache.Erase(entry);
        }
        return true;
    }

    if (!TransactionSignatureChecker::VerifySignature(vchSig, pubkey, sighash))
        return false;

    if (store)
    {
        signatureCache.Set(entry);
    }
    return true;
}
#endif

bool CachingTransactionSignatureChecker::VerifySignature(const std::vector<uint8_t> &vchSig,
    const CPubKey &pubkey,
    const uint256 &sighash) const
{
    return RunMemoizedCheck(vchSig, pubkey, sighash, nFlags, store,
        [&] { return TransactionSignatureChecker::VerifySignature(vchSig, pubkey, sighash); });
}
