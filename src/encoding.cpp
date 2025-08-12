#include "encoding.hpp"
#include "crypto.hpp"
#include "tx.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace axle {

static std::string b64(const bytes& v) {
    static const char* tbl="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val=0, valb=-6;
    for (uint8_t c: v) {
        val = (val<<8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(tbl[(val>>valb)&0x3F]);
            valb -= 6;
        }
    }
    if (valb>-6) out.push_back(tbl[((val<<8)>>(valb+8))&0x3F]);
    while (out.size()%4) out.push_back('=');
    return out;
}
static bytes b64d(const std::string& s) {
    static const int T[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,64,-1,-1,
        -1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
    };
    bytes out; out.reserve(s.size()*3/4);
    int val=0, valb=-8;
    for (unsigned char c : s) {
        if (T[c]==-1) break;
        if (T[c]==64) break;
        val = (val<<6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back((uint8_t)((val>>valb)&0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::string to_json(const SignedTx& tx) {
    json j;
    j["type"] = (int)tx.type;
    j["from"] = tx.from;
    j["to"] = tx.to;
    j["amount"] = tx.amount;
    j["nonce"] = tx.nonce;
    j["tokenId"] = tx.tokenId;
    j["meta"] = {{"name", tx.meta.name}, {"symbol", tx.meta.symbol}, {"uri", tx.meta.uri}};
    j["signature"] = b64(tx.signature);
    j["pubkey"] = b64(tx.pubkey);
    j["id"] = tx.id;
    return j.dump();
}

SignedTx tx_from_json(const std::string& js) {
    auto j = json::parse(js);
    SignedTx tx;
    tx.type = (TxType)((int)j.at("type"));
    tx.from = j.at("from");
    tx.to = j.at("to");
    tx.amount = j.at("amount");
    tx.nonce = j.at("nonce");
    tx.tokenId = j.value("tokenId", 0);
    auto m = j.at("meta");
    tx.meta = {m.value("name",""), m.value("symbol",""), m.value("uri","")};
    tx.signature = b64d(j.value("signature",""));
    tx.pubkey = b64d(j.value("pubkey",""));
    tx.id = j.value("id","");
    return tx;
}

std::string to_json(const Block& b) {
    json j;
    j["header"] = {
        {"height", b.header.height},
        {"prev_hash", b.header.prev_hash},
        {"merkle_root", b.header.merkle_root},
        {"timestamp", b.header.timestamp},
        {"difficulty_bits", b.header.difficulty_bits},
        {"nonce", b.header.nonce}
    };
    j["miner_address"] = b.miner_address;
    j["reward"] = b.reward;
    j["hash"] = b.hash;
    j["txs"] = json::array();
    for (auto& tx : b.txs) j["txs"].push_back(json::parse(to_json(tx)));
    return j.dump();
}

Block block_from_json(const std::string& js) {
    auto j = json::parse(js);
    Block b;
    auto h = j.at("header");
    b.header.height = h.at("height");
    b.header.prev_hash = h.at("prev_hash");
    b.header.merkle_root = h.at("merkle_root");
    b.header.timestamp = h.at("timestamp");
    b.header.difficulty_bits = h.at("difficulty_bits");
    b.header.nonce = h.at("nonce");
    b.miner_address = j.at("miner_address");
    b.reward = j.at("reward");
    b.hash = j.at("hash");
    b.txs.clear();
    for (auto& t : j.at("txs")) {
        b.txs.push_back(tx_from_json(t.dump()));
    }
    return b;
}

}
