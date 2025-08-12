#pragma once
#include "types.hpp"
#include "blockchain.hpp"
#include <string>
#include <thread>
#include <atomic>

namespace axle {

class RpcServer {
public:
    RpcServer(Blockchain& chain);
    ~RpcServer();

    bool start(const std::string& host, uint16_t port);
    void stop();
private:
    Blockchain& chain_;
    std::thread server_thread_;
    std::atomic<bool> running_{false};
};

}
