#include "miner.hpp"
#include "block.hpp"
#include "crypto.hpp"
#include <atomic>

namespace axle {

bool mine_block(Block& b, uint32_t difficulty_bits, uint64_t& iters) {
    iters = 0;
    for (;;) {
        b.header.nonce++;
        b.hash = block_hash(b.header);
        iters++;
        // quick check: leading zero bits
        auto h = unhex(b.hash);
        size_t zeros = 0;
        for (auto c: h) { if (c==0) zeros+=8; else { uint8_t v=c; while ((v&0x80)==0) { zeros++; v<<=1; } break; } }
        if (zeros >= difficulty_bits) return true;
        if (iters % 100000 == 0) return false; // yield to caller periodically
    }
}

}
