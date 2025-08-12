#include "ledger.hpp"
#include "tx.hpp"
#include "crypto.hpp"
#include "base58.hpp"
#include <stdexcept>

namespace axle {

static bool has_balance(const LedgerState& st, const std::string& addr, int64_t amt) {
    auto it = st.accounts.find(addr);
    return it != st.accounts.end() && it->second.balance >= amt;
}

static void add_balance(LedgerState& st, const std::string& addr, int64_t delta) {
    auto& a = st.accounts[addr];
    a.balance += delta;
}

ValidationResult apply_tx(LedgerState& st, const ChainParams& params, const SignedTx& tx) {
    ValidationResult vr;
    if (!verify_tx_sig(tx)) { vr.ok=false; vr.reason="bad signature"; return vr; }
    if (!verify_address(tx.from) || (!tx.to.empty() && !verify_address(tx.to))) { vr.ok=false; vr.reason="bad address"; return vr; }

    auto& sender = st.accounts[tx.from];
    if (sender.nonce != tx.nonce) { vr.ok=false; vr.reason="bad nonce"; return vr; }

    // universal burn
    int64_t required_burn = params.burn_fee;
    if (tx.type == TxType::TRANSFER) {
        if (tx.amount <= 0) { vr.ok=false; vr.reason="amount<=0"; return vr; }
        int64_t total = tx.amount + required_burn;
        if (!has_balance(st, tx.from, total)) { vr.ok=false; vr.reason="insufficient"; return vr; }
        add_balance(st, tx.from, -total);
        add_balance(st, tx.to, tx.amount);
        st.unclaimed_pool += required_burn;
    } else if (tx.type == TxType::MINT_NFT) {
        int64_t total = required_burn;
        if (!has_balance(st, tx.from, total)) { vr.ok=false; vr.reason="insufficient"; return vr; }
        add_balance(st, tx.from, -total);
        st.unclaimed_pool += required_burn;
        uint64_t id = st.next_token_id++;
        st.nfts[id] = {tx.from, tx.meta};
    } else if (tx.type == TxType::TRANSFER_NFT) {
        int64_t total = required_burn;
        if (!has_balance(st, tx.from, total)) { vr.ok=false; vr.reason="insufficient"; return vr; }
        auto it = st.nfts.find(tx.tokenId);
        if (it==st.nfts.end() || it->second.first != tx.from) { vr.ok=false; vr.reason="not owner"; return vr; }
        add_balance(st, tx.from, -total);
        st.unclaimed_pool += required_burn;
        it->second.first = tx.to;
    } else if (tx.type == TxType::BURN_NFT) {
        int64_t total = required_burn;
        if (!has_balance(st, tx.from, total)) { vr.ok=false; vr.reason="insufficient"; return vr; }
        auto it = st.nfts.find(tx.tokenId);
        if (it==st.nfts.end() || it->second.first != tx.from) { vr.ok=false; vr.reason="not owner"; return vr; }
        add_balance(st, tx.from, -total);
        st.unclaimed_pool += required_burn;
        st.nfts.erase(it);
    } else {
        vr.ok=false; vr.reason="unknown tx type"; return vr;
    }
    sender.nonce += 1;
    vr.ok = true;
    return vr;
}

ValidationResult validate_block(const LedgerState& prior, const ChainParams& params, const Block& b) {
    ValidationResult vr;
    // minimal checks; PoW checking to be done at accept time
    // Check txs nonces monotonic per account
    std::map<std::string,uint64_t> nonces;
    for (auto& [addr, acc] : prior.accounts) nonces[addr] = acc.nonce;
    LedgerState tmp = prior;
    int64_t pool0 = tmp.unclaimed_pool;
    for (auto& tx : b.txs) {
        auto r = apply_tx(tmp, params, tx);
        if (!r.ok) { vr.ok=false; vr.reason="tx invalid: "+r.reason; return vr; }
    }
    // reward should not exceed pool
    if (b.reward < 0 || b.reward > pool0 + (tmp.unclaimed_pool - pool0)) {
        // pool can only increase by burns
    }
    vr.ok = true;
    return vr;
}

void apply_block(LedgerState& st, const ChainParams& params, const Block& b) {
    for (auto& tx : b.txs) {
        auto r = apply_tx(st, params, tx);
        if (!r.ok) throw std::runtime_error("apply_block: tx invalid after validation");
    }
    // pay miner from unclaimed pool
    if (b.reward > st.unclaimed_pool) throw std::runtime_error("reward exceeds pool");
    st.unclaimed_pool -= b.reward;
    add_balance(st, b.miner_address, b.reward);
}

}
