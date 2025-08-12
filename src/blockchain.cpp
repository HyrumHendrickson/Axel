#include "blockchain.hpp"
#include "encoding.hpp"
#include "crypto.hpp"
#include "block.hpp"
#include "ledger.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <cmath>

namespace fs = std::filesystem;

namespace axle {

Blockchain::Blockchain(Storage& s, ChainParams p)
: storage_(s), params_(p) {}

bool Blockchain::init_genesis() {
    storage_.ensure_layout(params_);
    // genesis only if no blocks
    if (storage_.read_block(0).has_value()) return true;
    // ledger state already initialized with full pool
    storage_.save_state(LedgerState{}); // reset file first
    LedgerState st;
    storage_.load_state(st);
    state_ = st;
    Block genesis;
    genesis.header.height = 0;
    genesis.header.prev_hash = "";
    genesis.header.timestamp = std::chrono::duration_cast<std::chrono::seconds>(Clock::now().time_since_epoch()).count();
    genesis.header.difficulty_bits = difficulty_bits_;
    genesis.header.nonce = 0;
    genesis.header.merkle_root = "";
    genesis.reward = 0;
    genesis.miner_address = "";
    genesis.hash = block_hash(genesis.header);
    storage_.write_block(genesis);
    tip_height_ = 0;
    tip_hash_ = genesis.hash;
    storage_.write_tip(tip_height_, tip_hash_);
    storage_.save_state(state_);
    last_block_time_ = genesis.header.timestamp;
    return true;
}

bool Blockchain::load() {
    storage_.ensure_layout(params_);
    storage_.load_state(state_);
    auto tip = storage_.read_tip();
    if (tip) {
        tip_height_ = tip->first;
        tip_hash_ = tip->second;
    } else {
        return init_genesis();
    }
    // naive: read latest block for timestamp
    auto b = storage_.read_block(tip_height_);
    if (b) last_block_time_ = b->header.timestamp;
    return true;
}

static bool hash_meets_bits(const std::string& hexhash, uint32_t bits) {
    // Check leading zero bits
    size_t bytes_zero = bits / 8;
    uint8_t rem = bits % 8;
    auto h = unhex(hexhash);
    for (size_t i=0;i<bytes_zero;i++) if (h[i]!=0) return false;
    if (rem) {
        uint8_t mask = 0xFF << (8 - rem);
        if ((h[bytes_zero] & mask) != 0) return false;
    }
    return true;
}

Block Blockchain::build_block(const std::string& miner_addr, const std::vector<SignedTx>& txs) {
    Block b;
    b.header.height = tip_height_ + 1;
    b.header.prev_hash = tip_hash_;
    b.txs = txs;
    b.header.merkle_root = merkle_root(b.txs);
    b.header.timestamp = std::chrono::duration_cast<std::chrono::seconds>(Clock::now().time_since_epoch()).count();
    b.header.difficulty_bits = difficulty_bits_;
    b.miner_address = miner_addr;
    // compute target reward from pool and time remaining
    auto emission_end = last_block_time_ + params_.emission_years * 365 * 24 * 60 * 60;
    auto now = b.header.timestamp;
    int64_t remaining_secs = (emission_end > now) ? (emission_end - now) : params_.target_block_time_sec;
    int64_t remaining_blocks = std::max<int64_t>(1, remaining_secs / params_.target_block_time_sec);
    int64_t reward = state_.unclaimed_pool / remaining_blocks;
    if (reward < 0) reward = 0;
    b.reward = reward;
    return b;
}

bool Blockchain::accept_block(const Block& b) {
    // Basic checks
    if (b.header.height != tip_height_ + 1) return false;
    if (b.header.prev_hash != tip_hash_) return false;
    if (!hash_meets_bits(b.hash, b.header.difficulty_bits)) return false;
    // validate txs and reward
    auto vr = validate_block(state_, params_, b);
    if (!vr.ok) return false;

    // apply
    apply_block(state_, params_, b);
    tip_height_ = b.header.height;
    tip_hash_ = b.hash;
    storage_.write_block(b);
    storage_.write_tip(tip_height_, tip_hash_);
    storage_.save_state(state_);

    // adjust difficulty +/- 1 bit based on timing
    uint64_t now = b.header.timestamp;
    uint64_t dt = (last_block_time_==0) ? params_.target_block_time_sec : (now - last_block_time_);
    last_block_time_ = now;
    if (dt < (uint64_t)params_.target_block_time_sec/2 && difficulty_bits_ < 31) difficulty_bits_++;
    else if (dt > (uint64_t)params_.target_block_time_sec*2 && difficulty_bits_ > 8) difficulty_bits_--;

    return true;
}

}
