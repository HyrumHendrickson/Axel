#pragma once
#include "types.hpp"
#include <string>

namespace axle {

std::string header_preimage(const BlockHeader& h);
std::string block_hash(const BlockHeader& h);
std::string merkle_root(const std::vector<SignedTx>& txs);

}
