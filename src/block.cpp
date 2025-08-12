#include "block.hpp"
#include "encoding.hpp"
#include "crypto.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace axle {

static bytes to_bytes(const std::string& s) { return bytes(s.begin(), s.end()); }

std::string header_preimage(const BlockHeader& h) {
    json j;
    j["height"] = h.height;
    j["prev_hash"] = h.prev_hash;
    j["merkle_root"] = h.merkle_root;
    j["timestamp"] = h.timestamp;
    j["difficulty_bits"] = h.difficulty_bits;
    j["nonce"] = h.nonce;
    return j.dump();
}

std::string block_hash(const BlockHeader& h) {
    auto d = double_sha256(to_bytes(header_preimage(h)));
    return hex(d);
}

static std::string hash_concat(const std::string& a, const std::string& b) {
    bytes x = bytes(a.begin(), a.end());
    bytes y = bytes(b.begin(), b.end());
    return hex(double_sha256(bytes{x.begin(), x.end()}));
}

std::string merkle_root(const std::vector<SignedTx>& txs) {
    if (txs.empty()) return "";
    std::vector<std::string> level;
    level.reserve(txs.size());
    for (auto& tx : txs) {
        auto h = double_sha256(bytes(tx.id.begin(), tx.id.end()));
        level.push_back(hex(h));
    }
    while (level.size() > 1) {
        std::vector<std::string> next;
        for (size_t i=0;i<level.size();i+=2) {
            if (i+1<level.size()) {
                bytes d(level[i].begin(), level[i].end());
                d.insert(d.end(), level[i+1].begin(), level[i+1].end());
                next.push_back(hex(double_sha256(d)));
            } else {
                next.push_back(level[i]);
            }
        }
        level.swap(next);
    }
    return level[0];
}

}
