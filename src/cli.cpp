#include "cli.hpp"
#include "crypto.hpp"
#include "storage.hpp"
#include "blockchain.hpp"
#include "miner.hpp"
#include "encoding.hpp"
#include "tx.hpp"
#include "p2p.hpp"
#include "rpc.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace axle {

static void usage() {
    std::cout << "Axle Chain CLI\n"
              << "  init --datadir DIR [--network mainnet]\n"
              << "  start --datadir DIR [--p2p HOST:PORT] [--rpc HOST:PORT] [--bootstrap HOST:PORT]\n"
              << "  create-address --datadir DIR --name NAME\n"
              << "  send --datadir DIR --from NAME --to ADDR --amount N.NNNNNNNN\n"
              << "  mine --datadir DIR\n"
              << "  mint-nft --datadir DIR --from NAME --name NAME --symbol SYM --uri URI\n"
              << std::endl;
}

static std::string join(const std::string& a, const std::string& b) {
    return (fs::path(a)/b).string();
}

static bool load_keys(const std::string& datadir, const std::string& name, bytes& priv, bytes& pub, std::string& addr) {
    fs::path p = fs::path(datadir) / "keys" / (name + ".json");
    if (!fs::exists(p)) return false;
    std::ifstream f(p);
    json j; f >> j;
    auto privhex = j["priv"].get<std::string>();
    auto pubhex  = j["pub"].get<std::string>();
    priv = unhex(privhex);
    pub = unhex(pubhex);
    addr = j["address"].get<std::string>();
    return true;
}

static bool save_keys(const std::string& datadir, const std::string& name, const bytes& priv, const bytes& pub) {
    fs::create_directories(fs::path(datadir)/"keys");
    std::string addr = address_from_pubkey(pub);
    json j = {{"priv", hex(priv)}, {"pub", hex(pub)}, {"address", addr}};
    std::ofstream(fs::path(datadir)/"keys"/(name+".json")) << j.dump(2);
    std::cout << "Saved key '"<<name<<"' address: " << addr << std::endl;
    return true;
}

int run_cli(int argc, char** argv) {
    if (argc < 2) { usage(); return 1; }
    sodium_init_or_throw();

    std::string cmd = argv[1];
    std::string datadir = "./data";
    std::string network = "mainnet";
    std::string p2p = "0.0.0.0:9735";
    std::string rpc = "127.0.0.1:9736";
    std::string bootstrap = "";

    // simple arg parse
    for (int i=2;i<argc;i++) {
        std::string a = argv[i];
        auto val = [&](){ return (i+1<argc)?std::string(argv[++i]):std::string(); };
        if (a=="--datadir") datadir = val();
        else if (a=="--network") network = val();
        else if (a=="--p2p") p2p = val();
        else if (a=="--rpc") rpc = val();
        else if (a=="--bootstrap") bootstrap = val();
        else if (a=="--help") { usage(); return 0; }
    }

    ChainParams params; params.network = network;

    if (cmd=="init") {
        Storage st(datadir);
        st.ensure_layout(params);
        Blockchain chain(st, params);
        chain.init_genesis();
        fs::create_directories(fs::path(datadir)/"keys");
        auto kp = keygen();
        save_keys(datadir, "default", kp.priv, kp.pub);
        std::cout << "Initialized datadir at " << datadir << std::endl;
        return 0;
    } else if (cmd=="create-address") {
        std::string name;
        for (int i=2;i<argc;i++) if (std::string(argv[i])=="--name" && i+1<argc) name=argv[i+1];
        if (name.empty()) { std::cerr << "--name required\n"; return 1; }
        auto kp = keygen();
        save_keys(datadir, name, kp.priv, kp.pub);
        return 0;
    } else if (cmd=="start") {
        Storage st(datadir);
        Blockchain chain(st, params);
        chain.load();
        P2PNode p2pnode(chain);
        // parse host:port
        auto split = [](const std::string& hp){ auto pos=hp.find(':'); return std::make_pair(hp.substr(0,pos), (uint16_t)std::stoi(hp.substr(pos+1))); };
        auto [host,port] = split(p2p);
        p2pnode.start_listen(host, port);
        if (!bootstrap.empty()) {
            auto [bh,bp] = split(bootstrap);
            p2pnode.add_peer(bh,bp);
        }
        RpcServer rpcserver(chain);
        auto [rh,rp] = split(rpc);
        rpcserver.start(rh,rp);
        std::cout << "Node started. Press Ctrl+C to exit.\n";
        while (true) std::this_thread::sleep_for(std::chrono::seconds(10));
        return 0;
    } else if (cmd=="send") {
        std::string from, to; double amount=0.0;
        for (int i=2;i<argc;i++) {
            std::string a = argv[i];
            if (a=="--from") from = argv[++i];
            else if (a=="--to") to = argv[++i];
            else if (a=="--amount") amount = std::stod(argv[++i]);
        }
        if (from.empty()||to.empty()) { std::cerr << "--from,--to required\n"; return 1; }
        bytes priv,pub; std::string addr;
        if (!load_keys(datadir, from, priv, pub, addr)) { std::cerr << "no keys for "<<from<<"\n"; return 1; }
        Storage st(datadir);
        Blockchain chain(st, params);
        chain.load();
        // determine nonce
        LedgerState stt; st.load_state(stt);
        uint64_t nonce = stt.accounts[addr].nonce;
        SignedTx utx;
        utx.type = TxType::TRANSFER;
        utx.from = addr; utx.to = to; utx.amount = (int64_t)llround(amount * UNIT); utx.nonce = nonce;
        auto tx = sign_tx(utx, priv);
        // For simplicity, include tx in a new block and mine locally
        auto blk = chain.build_block(addr, {tx});
        uint64_t iters=0;
        while (true) {
            if (mine_block(blk, chain.current_difficulty_bits(), iters)) {
                if (chain.accept_block(blk)) {
                    std::cout << "Mined and accepted block " << blk.header.height << " hash="<<blk.hash<<"\n";
                    break;
                }
            }
        }
        return 0;
    } else if (cmd=="mine") {
        bytes priv,pub; std::string addr;
        if (!load_keys(datadir, "default", priv, pub, addr)) { std::cerr << "no default key\n"; return 1; }
        Storage st(datadir);
        Blockchain chain(st, params);
        chain.load();
        auto blk = chain.build_block(addr, {});
        uint64_t iters=0;
        while (true) {
            if (mine_block(blk, chain.current_difficulty_bits(), iters)) {
                if (chain.accept_block(blk)) {
                    std::cout << "Mined and accepted block " << blk.header.height << " hash="<<blk.hash<<"\n";
                    break;
                }
            }
        }
        return 0;
    } else if (cmd=="mint-nft") {
        std::string from, name, sym, uri;
        for (int i=2;i<argc;i++) {
            std::string a = argv[i];
            if (a=="--from") from = argv[++i];
            else if (a=="--name") name = argv[++i];
            else if (a=="--symbol") sym = argv[++i];
            else if (a=="--uri") uri = argv[++i];
        }
        if (from.empty()||name.empty()) { std::cerr << "--from and --name required\n"; return 1; }
        bytes priv,pub; std::string addr;
        if (!load_keys(datadir, from, priv, pub, addr)) { std::cerr << "no keys for "<<from<<"\n"; return 1; }
        Storage st(datadir);
        Blockchain chain(st, params);
        chain.load();
        LedgerState stt; st.load_state(stt);
        uint64_t nonce = stt.accounts[addr].nonce;
        SignedTx utx;
        utx.type = TxType::MINT_NFT;
        utx.from = addr; utx.to = addr; utx.amount = 0; utx.nonce = nonce;
        utx.meta = {name, sym, uri};
        auto tx = sign_tx(utx, priv);
        auto blk = chain.build_block(addr, {tx});
        uint64_t iters=0;
        while (true) {
            if (mine_block(blk, chain.current_difficulty_bits(), iters)) {
                if (chain.accept_block(blk)) {
                    std::cout << "Mined and accepted block " << blk.header.height << " hash="<<blk.hash<<"\n";
                    break;
                }
            }
        }
        return 0;
    } else {
        usage();
        return 1;
    }
}

}
