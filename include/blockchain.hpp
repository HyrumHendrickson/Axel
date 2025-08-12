#pragma once
#include "types.hpp"
#include "storage.hpp"

namespace axle {

class Blockchain {
public:
    Blockchain(Storage& s, ChainParams p);
    bool init_genesis();
    bool load();
    const ChainParams& params() const { return params_; }
    const LedgerState& state() const { return state_; }
    uint64_t tip_height() const { return tip_height_; }
    std::string tip_hash() const { return tip_hash_; }

    bool accept_block(const Block& b);
    Block build_block(const std::string& miner_addr, const std::vector<SignedTx>& txs);

    // simple difficulty control
    uint32_t current_difficulty_bits() const { return difficulty_bits_; }
private:
    Storage& storage_;
    ChainParams params_;
    LedgerState state_;
    uint64_t tip_height_{0};
    std::string tip_hash_{};
    uint32_t difficulty_bits_{18};
    uint64_t last_block_time_{0};
};

}
