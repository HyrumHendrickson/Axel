#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "crypto.hpp"
#include "base58.hpp"
#include "tx.hpp"

using namespace axle;

TEST_CASE("keygen and address") {
    sodium_init_or_throw();
    auto kp = keygen();
    auto addr = address_from_pubkey(kp.pub);
    CHECK(verify_address(addr));
}

TEST_CASE("sign and verify tx") {
    sodium_init_or_throw();
    auto kp = keygen();
    SignedTx tx;
    tx.type = TxType::TRANSFER;
    tx.from = address_from_pubkey(kp.pub);
    tx.to = tx.from;
    tx.amount = 1234;
    tx.nonce = 0;
    auto stx = sign_tx(tx, kp.priv);
    CHECK(verify_tx_sig(stx));
}
