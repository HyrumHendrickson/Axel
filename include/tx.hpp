#pragma once
#include "types.hpp"
#include <string>

namespace axle {

std::string tx_preimage(const SignedTx& tx); // message to sign
SignedTx sign_tx(const SignedTx& unsignedTx, const std::vector<uint8_t>& priv);
bool verify_tx_sig(const SignedTx& tx);
std::string tx_id(const SignedTx& tx);

}
