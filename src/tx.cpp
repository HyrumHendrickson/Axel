#include "tx.hpp"
#include "encoding.hpp"
#include "crypto.hpp"
#include "base58.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace axle {

std::string tx_preimage(const SignedTx& tx) {
    json j;
    j["type"] = (int)tx.type;
    j["from"] = tx.from;
    j["to"] = tx.to;
    j["amount"] = tx.amount;
    j["nonce"] = tx.nonce;
    j["tokenId"] = tx.tokenId;
    j["meta"] = {{"name", tx.meta.name}, {"symbol", tx.meta.symbol}, {"uri", tx.meta.uri}};
    return j.dump();
}

SignedTx sign_tx(const SignedTx& unsignedTx, const std::vector<uint8_t>& priv) {
    SignedTx tx = unsignedTx;
    auto msg = bytes(tx_preimage(tx).begin(), tx_preimage(tx).end());
    tx.signature = ed25519_sign(msg, priv);
    tx.pubkey.resize(crypto_sign_PUBLICKEYBYTES);
    // libsodium secret key ends with pubkey
    for (size_t i=0;i<crypto_sign_PUBLICKEYBYTES;i++) tx.pubkey[i] = priv[crypto_sign_PUBLICKEYBYTES + i];
    tx.id = hex(double_sha256(msg));
    return tx;
}

bool verify_tx_sig(const SignedTx& tx) {
    if (!verify_address(tx.from) || !verify_address(tx.to)) return false;
    auto msg = bytes(tx_preimage(tx).begin(), tx_preimage(tx).end());
    return ed25519_verify(msg, tx.signature, tx.pubkey);
}

std::string tx_id(const SignedTx& tx) {
    return tx.id;
}

}
