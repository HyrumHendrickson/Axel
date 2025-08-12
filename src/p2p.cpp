#include "p2p.hpp"
#include "encoding.hpp"
#include "crypto.hpp"
#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace axle {

P2PNode::P2PNode(Blockchain& chain) : chain_(chain) {}
P2PNode::~P2PNode() { stop(); }

bool P2PNode::start_listen(const std::string& host, uint16_t port) {
    if (running_) return false;
    running_ = true;
    server_thread_ = std::thread([this, host, port](){
        try {
            asio::io_context io;
            asio::ip::tcp::acceptor acc(io, asio::ip::tcp::endpoint(asio::ip::make_address(host), port));
            while (running_) {
                asio::ip::tcp::socket sock(io);
                acc.accept(sock);
                // Very simple: on connect, send tip height
                auto tip = chain_.tip_height();
                json j = {{"type","hello"},{"height",tip}};
                std::string s = j.dump()+"\n";
                asio::write(sock, asio::buffer(s));
            }
        } catch (std::exception& e) {
            std::cerr << "[P2P] listen error: " << e.what() << std::endl;
        }
    });
    return true;
}

void P2PNode::add_peer(const std::string& host, uint16_t port) {
    peers_.push_back({host, port});
}

void P2PNode::stop() {
    if (!running_) return;
    running_ = false;
    if (server_thread_.joinable()) server_thread_.join();
}

void P2PNode::broadcast_block(const Block& b) {
    // naive: connect and send serialized block to each peer
    for (auto& [h,p] : peers_) {
        try {
            asio::io_context io;
            asio::ip::tcp::socket sock(io);
            sock.connect({asio::ip::make_address(h), p});
            json j = {{"type","block"}, {"data", json::parse(to_json(b))}};
            std::string s = j.dump()+"\n";
            asio::write(sock, asio::buffer(s));
        } catch (...) {}
    }
}

}
