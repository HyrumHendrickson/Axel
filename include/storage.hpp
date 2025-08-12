#pragma once
#include "types.hpp"
#include <string>
#include <optional>

namespace axle {

class Storage {
    std::string datadir_;
public:
    explicit Storage(std::string datadir);
    bool ensure_layout(const ChainParams& params);
    std::optional<Block> read_block(uint64_t height) const;
    bool write_block(const Block& b) const;
    bool write_tip(uint64_t height, const std::string& hash) const;
    std::optional<std::pair<uint64_t,std::string>> read_tip() const;
    bool load_state(LedgerState& st) const;
    bool save_state(const LedgerState& st) const;
    std::string blocks_dir() const;
};

}
