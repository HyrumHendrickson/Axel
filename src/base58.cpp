#include "base58.hpp"
#include "crypto.hpp"
#include <array>
#include <algorithm>

namespace axle {

static const char* ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

std::string base58_encode(const std::vector<uint8_t>& data) {
    // Big integer base conversion from base256 to base58
    std::vector<uint8_t> in = data;
    size_t zeros = 0;
    while (zeros < in.size() && in[zeros] == 0) zeros++;

    std::vector<uint8_t> b58((in.size() - zeros) * 138 / 100 + 1);
    size_t length = 0;
    for (size_t i = zeros; i < in.size(); ++i) {
        int carry = in[i];
        size_t j = 0;
        for (auto it = b58.rbegin(); (carry != 0 || j < length) && it != b58.rend(); ++it, ++j) {
            carry += 256 * (*it);
            *it = carry % 58;
            carry /= 58;
        }
        length = j;
    }
    auto it = std::find_if(b58.begin(), b58.end(), [](uint8_t c){ return c != 0; });
    std::string s;
    s.assign(zeros, '1');
    while (it != b58.end()) {
        s += ALPHABET[*it];
        ++it;
    }
    return s;
}

std::vector<uint8_t> base58_decode(const std::string& s, bool* ok) {
    std::vector<uint8_t> out;
    std::vector<uint8_t> b256((s.size()) * 733 / 1000 + 1);
    std::vector<int> map(256, -1);
    for (int i=0;i<58;i++) map[(int)ALPHABET[i]] = i;

    size_t zeros = 0;
    while (zeros < s.size() && s[zeros] == '1') zeros++;

    size_t length = 0;
    for (size_t i = zeros; i < s.size(); ++i) {
        int c = (unsigned char)s[i];
        if (map[c] == -1) { if (ok) *ok=false; return {}; }
        int carry = map[c];
        size_t j = 0;
        for (auto it = b256.rbegin(); (carry != 0 || j < length) && it != b256.rend(); ++it, ++j) {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
        }
        length = j;
    }
    auto it = std::find_if(b256.begin(), b256.end(), [](uint8_t c){ return c != 0; });
    out.assign(zeros, 0);
    while (it != b256.end()) { out.push_back(*it); ++it; }
    if (ok) *ok=true;
    return out;
}

std::string base58check_encode(uint8_t version, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> data;
    data.push_back(version);
    data.insert(data.end(), payload.begin(), payload.end());
    auto checksum = double_sha256(data);
    data.insert(data.end(), checksum.begin(), checksum.begin()+4);
    return base58_encode(data);
}

bool base58check_decode(const std::string& s, uint8_t& version_out, std::vector<uint8_t>& payload_out) {
    bool ok;
    auto data = base58_decode(s, &ok);
    if (!ok || data.size() < 5) return false;
    version_out = data[0];
    payload_out.assign(data.begin()+1, data.end()-4);
    std::vector<uint8_t> check_input(data.begin(), data.end()-4);
    auto checksum = double_sha256(check_input);
    return std::equal(checksum.begin(), checksum.begin()+4, data.end()-4);
}

}
