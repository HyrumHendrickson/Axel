#pragma once
#include "types.hpp"
#include "blockchain.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <vector>

namespace axle {

class P2PNode {
public:
    P2PNode(Blockchain& chain);
    ~P2PNode();

    bool start_listen(const std::string& host, uint16_t port);
    void add_peer(const std::string& host, uint16_t port);
    void stop();

    void broadcast_block(const Block& b);
private:
    Blockchain& chain_;
    std::thread server_thread_;
    std::atomic<bool> running_{false};
    std::vector<std::pair<std::string,uint16_t>> peers_;
};

}
