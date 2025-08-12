#include "storage.hpp"
#include "encoding.hpp"
#include "crypto.hpp"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace axle {

Storage::Storage(std::string datadir): datadir_(std::move(datadir)) {}

std::string Storage::blocks_dir() const { return (fs::path(datadir_) / "blocks").string(); }

bool Storage::ensure_layout(const ChainParams& params) {
    fs::create_directories(datadir_);
    fs::create_directories(blocks_dir());
    // Initialize state if not present
    fs::path state_path = fs::path(datadir_) / "state.json";
    if (!fs::exists(state_path)) {
        LedgerState st;
        st.unclaimed_pool = params.supply_cap;
        json j;
        j["accounts"] = json::object();
        j["nfts"] = json::object();
        j["next_token_id"] = st.next_token_id;
        j["unclaimed_pool"] = st.unclaimed_pool;
        std::ofstream(state_path) << j.dump(2);
    }
    return true;
}

std::optional<Block> Storage::read_block(uint64_t height) const {
    fs::path p = fs::path(blocks_dir()) / (std::to_string(height) + ".json");
    if (!fs::exists(p)) return std::nullopt;
    std::ifstream f(p);
    std::string s((std::istreambuf_iterator<char>(f)), {});
    return block_from_json(s);
}

bool Storage::write_block(const Block& b) const {
    fs::path p = fs::path(blocks_dir()) / (std::to_string(b.header.height) + ".json");
    std::ofstream f(p);
    f << to_json(b);
    return true;
}

bool Storage::write_tip(uint64_t height, const std::string& hash) const {
    fs::path p = fs::path(datadir_) / "tip.json";
    json j; j["height"] = height; j["hash"] = hash;
    std::ofstream(p) << j.dump(2);
    return true;
}

std::optional<std::pair<uint64_t,std::string>> Storage::read_tip() const {
    fs::path p = fs::path(datadir_) / "tip.json";
    if (!fs::exists(p)) return std::nullopt;
    std::ifstream f(p);
    json j; f >> j;
    return std::make_pair((uint64_t)j["height"], (std::string)j["hash"]);
}

bool Storage::load_state(LedgerState& st) const {
    fs::path p = fs::path(datadir_) / "state.json";
    if (!fs::exists(p)) return false;
    std::ifstream f(p);
    json j; f >> j;
    st.accounts.clear();
    for (auto it = j["accounts"].begin(); it != j["accounts"].end(); ++it) {
        AccountState a; a.balance = it.value()["balance"]; a.nonce = it.value()["nonce"];
        st.accounts[it.key()] = a;
    }
    st.nfts.clear();
    if (j.contains("nfts")) {
        for (auto it = j["nfts"].begin(); it != j["nfts"].end(); ++it) {
            uint64_t id = std::stoull(it.key());
            std::string owner = it.value()["owner"];
            NFTMeta meta{it.value()["meta"]["name"], it.value()["meta"]["symbol"], it.value()["meta"]["uri"]};
            st.nfts[id] = {owner, meta};
        }
    }
    st.next_token_id = j.value("next_token_id", 1);
    st.unclaimed_pool = j.value("unclaimed_pool", 0);
    return true;
}

bool Storage::save_state(const LedgerState& st) const {
    fs::path p = fs::path(datadir_) / "state.json";
    json j;
    j["accounts"] = json::object();
    for (auto& [addr, a] : st.accounts) {
        j["accounts"][addr] = {{"balance", a.balance}, {"nonce", a.nonce}};
    }
    j["nfts"] = json::object();
    for (auto& [id, pair] : st.nfts) {
        j["nfts"][std::to_string(id)] = {
            {"owner", pair.first},
            {"meta", {{"name", pair.second.name}, {"symbol", pair.second.symbol}, {"uri", pair.second.uri}}}
        };
    }
    j["next_token_id"] = st.next_token_id;
    j["unclaimed_pool"] = st.unclaimed_pool;
    std::ofstream(p) << j.dump(2);
    return true;
}

}
