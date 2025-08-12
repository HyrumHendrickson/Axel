#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace axle {

std::string base58_encode(const std::vector<uint8_t>& data);
std::vector<uint8_t> base58_decode(const std::string& s, bool* ok=nullptr);
std::string base58check_encode(uint8_t version, const std::vector<uint8_t>& payload);
bool base58check_decode(const std::string& s, uint8_t& version_out, std::vector<uint8_t>& payload_out);

}
