#pragma once
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace axle {

using bytes = std::vector<uint8_t>;

struct KeyPair {
    bytes pub;  // 32 bytes
    bytes priv; // 64 bytes (ed25519 secret+public concatenated as libsodium provides)
};

void sodium_init_or_throw();
KeyPair keygen();
bytes ed25519_sign(const bytes& msg, const bytes& priv);
bool ed25519_verify(const bytes& msg, const bytes& sig, const bytes& pub);

bytes sha256(const bytes& data);
bytes double_sha256(const bytes& data);
std::string hex(const bytes& v);
bytes unhex(const std::string& s);
bytes random_bytes(size_t n);

std::string address_from_pubkey(const bytes& pub);
bool verify_address(const std::string& addr);

}
