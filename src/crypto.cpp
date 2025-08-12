#include "crypto.hpp"
#include "base58.hpp"
#include <sodium.h>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace axle {

void sodium_init_or_throw() {
    if (sodium_init() < 0) throw std::runtime_error("libsodium init failed");
}

KeyPair keygen() {
    KeyPair kp;
    kp.pub.resize(crypto_sign_PUBLICKEYBYTES);
    kp.priv.resize(crypto_sign_SECRETKEYBYTES);
    crypto_sign_keypair(kp.pub.data(), kp.priv.data());
    return kp;
}

bytes ed25519_sign(const bytes& msg, const bytes& priv) {
    bytes sig(crypto_sign_BYTES);
    if (crypto_sign_detached(sig.data(), nullptr, msg.data(), msg.size(), priv.data()) != 0) {
        throw std::runtime_error("sign failed");
    }
    return sig;
}

bool ed25519_verify(const bytes& msg, const bytes& sig, const bytes& pub) {
    return crypto_sign_verify_detached(sig.data(), msg.data(), msg.size(), pub.data()) == 0;
}

bytes sha256(const bytes& data) {
    bytes out(crypto_hash_sha256_BYTES);
    crypto_hash_sha256(out.data(), data.data(), data.size());
    return out;
}

bytes double_sha256(const bytes& data) {
    return sha256(sha256(data));
}

std::string hex(const bytes& v) {
    std::ostringstream oss;
    for (auto b: v) { oss << std::hex << std::setw(2) << std::setfill('0') << (int)b; }
    return oss.str();
}

static uint8_t fromhex(char c) {
    if (c>='0'&&c<='9') return c-'0';
    if (c>='a'&&c<='f') return 10+(c-'a');
    if (c>='A'&&c<='F') return 10+(c-'A');
    return 0;
}

bytes unhex(const std::string& s) {
    bytes out;
    out.reserve(s.size()/2);
    for (size_t i=0;i+1<s.size();i+=2) {
        out.push_back((fromhex(s[i])<<4) | fromhex(s[i+1]));
    }
    return out;
}

bytes random_bytes(size_t n) {
    bytes b(n);
    randombytes_buf(b.data(), b.size());
    return b;
}

std::string address_from_pubkey(const bytes& pub) {
    auto h = sha256(pub);
    bytes payload(h.begin(), h.begin()+20);
    return base58check_encode(23, payload);
}

bool verify_address(const std::string& addr) {
    uint8_t ver; std::vector<uint8_t> payload;
    if (!base58check_decode(addr, ver, payload)) return false;
    if (ver != 23 || payload.size()!=20) return false;
    return true;
}

}
