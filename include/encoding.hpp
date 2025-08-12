#pragma once
#include "types.hpp"
#include <string>
#include <vector>

namespace axle {

std::string to_json(const SignedTx& tx);
SignedTx tx_from_json(const std::string& js);
std::string to_json(const Block& b);
Block block_from_json(const std::string& js);

}
