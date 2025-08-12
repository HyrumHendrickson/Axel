#pragma once
#include "types.hpp"

namespace axle {

struct ValidationResult {
    bool ok{true};
    std::string reason;
};

ValidationResult apply_tx(LedgerState& st, const ChainParams& params, const SignedTx& tx);
ValidationResult validate_block(const LedgerState& prior, const ChainParams& params, const Block& b);
void apply_block(LedgerState& st, const ChainParams& params, const Block& b);

}
