#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <optional>
#include <chrono>
#include <map>

namespace axle {

using bytes = std::vector<uint8_t>;
using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock>;

static constexpr int64_t DECIMALS = 8;
static constexpr int64_t UNIT = 100000000; // 1 AXLE = 1e8
static constexpr int64_t BURN_FEE_UNITS = 1000000; // 0.01 AXLE
static constexpr int ADDRESS_VERSION = 23; // Base58Check version byte

enum class TxType : uint8_t {
    TRANSFER = 0,
    MINT_NFT = 1,
    TRANSFER_NFT = 2,
    BURN_NFT = 3
};

struct NFTMeta {
    std::string name;
    std::string symbol;
    std::string uri; // IPFS/HTTPS
};

struct SignedTx {
    TxType type;
    std::string from;
    std::string to;
    int64_t amount{0}; // in nano-AXLE
    uint64_t nonce{0};
    uint64_t tokenId{0}; // for NFT ops
    NFTMeta meta;
    bytes signature; // ed25519 signature
    bytes pubkey;    // ed25519 public key of sender

    std::string id; // txid (hex of double SHA-256 over serialization)
};

struct BlockHeader {
    uint64_t height{0};
    std::string prev_hash;
    std::string merkle_root;
    uint64_t timestamp{0}; // unix
    uint32_t difficulty_bits{18}; // number of leading zero bits required
    uint64_t nonce{0};
};

struct Block {
    BlockHeader header;
    std::vector<SignedTx> txs;
    std::string hash; // hex
    std::string miner_address; // coinbase destination
    int64_t reward{0}; // reward paid to miner from pool
};

struct AccountState {
    int64_t balance{0};
    uint64_t nonce{0};
};

struct ChainParams {
    std::string network{"mainnet"};
    uint32_t network_id{0xA117E};
    int64_t supply_cap{100000000000LL * UNIT};
    int64_t burn_fee{BURN_FEE_UNITS};
    int target_block_time_sec{30};
    int emission_years{8};
};

struct LedgerState {
    std::map<std::string, AccountState> accounts;
    std::map<uint64_t, std::pair<std::string, NFTMeta>> nfts; // tokenId -> (owner, meta)
    uint64_t next_token_id{1};
    int64_t unclaimed_pool{0}; // starts at supply cap
};

} // namespace axle
