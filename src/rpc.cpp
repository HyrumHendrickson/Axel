#include "rpc.hpp"
#include "encoding.hpp"
#include "crypto.hpp"
#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace axle {

RpcServer::RpcServer(Blockchain& chain) : chain_(chain) {}
RpcServer::~RpcServer() { stop(); }

bool RpcServer::start(const std::string& host, uint16_t port) {
    if (running_) return false;
    running_ = true;
    server_thread_ = std::thread([this, host, port](){
        try {
            asio::io_context io;
            asio::ip::tcp::acceptor acc(io, asio::ip::tcp::endpoint(asio::ip::make_address(host), port));
            while (running_) {
                asio::ip::tcp::socket sock(io);
                acc.accept(sock);
                // extremely simple: read first line and respond
                asio::streambuf buf;
                asio::read_until(sock, buf, "\n");
                std::istream is(&buf);
                std::string line; std::getline(is, line);
                json req = json::parse(line, nullptr, false);
                json resp;
                if (req.is_discarded()) {
                    resp = {{"error","bad json"}};
                } else if (req["method"]=="get_tip") {
                    resp = {{"height", chain_.tip_height()}, {"hash", chain_.tip_hash()}};
                } else {
                    resp = {{"error","unknown method"}};
                }
                std::string s = resp.dump()+"\n";
                asio::write(sock, asio::buffer(s));
            }
        } catch (std::exception& e) {
            std::cerr << "[RPC] error: " << e.what() << std::endl;
        }
    });
    return true;
}

void RpcServer::stop() {
    if (!running_) return;
    running_ = false;
    if (server_thread_.joinable()) server_thread_.join();
}

}
