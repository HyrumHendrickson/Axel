# Axle Chain

A basic but functional cryptocurrency and NFT chain for teaching and experimentation.  
**Language:** C++20 | **Platforms:** Linux, Windows | **License:** MIT

## Highlights
- New standalone chain with real P2P networking (Asio TCP) and simple HTTP JSON-RPC.
- Proof-of-Work (double SHA-256) with per-block difficulty retarget toward a 30-second target.
- Account-based ledger (8 decimals), Base58Check addresses, ed25519 signatures (libsodium).
- **Fee/Burn rule:** every transaction burns exactly `0.01 AXLE` which is added to the **Unclaimed Pool**.
- **Monetary model:** hard cap `100,000,000,000.00000000 AXLE`. The Unclaimed Pool starts at the full supply.
  Miners are paid **from the pool** with an emission schedule that aims to distribute coins evenly over ~8 years
  (reward recalculated each block as `pool / remaining_blocks`). Additional burns replenish the pool.
- Minimal NFT support: mint, transfer, burn; metadata fields `{name, symbol, uri}`.
- Simple persistent storage using append-only block files + JSON state (to keep the build light for classes).
  (A SQLite backend can be added later; this code keeps the storage layer isolated.)

> This is a **reference/teaching** implementation. It has guardrails and basic validation, but it is **not
> production security**. Use on private/test networks for education and research.

## Quick build

### Dependencies
- **CMake ≥ 3.20**
- **C++20** compiler (GCC 12+, Clang 15+, or MSVC 2022+)
- **libsodium** (ed25519 and SHA-256)
- **Asio** (header-only) – fetched automatically via CMake FetchContent
- **nlohmann/json** (header-only) – fetched automatically via CMake FetchContent

On Debian/Ubuntu:
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libsodium-dev
```

On Fedora:
```bash
sudo dnf install -y gcc-c++ cmake libsodium-devel
```

On Windows (MSVC):
- Install CMake and Visual Studio 2022.
- Install vcpkg (optional) and `vcpkg install libsodium` then set `CMAKE_TOOLCHAIN_FILE` accordingly,
  **or** install libsodium via binary release and make sure `sodium` is discoverable by CMake.

### Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The `axle` executable is produced in `build/axle` (or `build\Debug\axle.exe` on Windows).

## Usage

Initialize a node (creates a data dir with genesis and a key):
```bash
./build/axle init --datadir ./data --network mainnet
```

Start the node (P2P + JSON-RPC):
```bash
./build/axle start --datadir ./data --p2p 0.0.0.0:9735 --rpc 127.0.0.1:9736 --bootstrap 127.0.0.1:9735
```

Create an address:
```bash
./build/axle create-address --datadir ./data --name alice
```

Send a transaction (burns 0.01 AXLE automatically):
```bash
./build/axle send --datadir ./data --from alice --to <Address> --amount 12.50000000
```

Mine one block immediately (useful for demos):
```bash
./build/axle mine --datadir ./data
```

Mint an NFT:
```bash
./build/axle mint-nft --datadir ./data --from alice --name "Hello" --symbol "HLO" --uri "ipfs://..."
```

### JSON-RPC (localhost by default)
- `GET /get_balance?address=<addr>`
- `POST /send_tx` (JSON body: a serialized signed transaction)
- `GET /get_block?height=N`
- `GET /get_tip`
- `GET /get_nft?id=123`

## Configuration
See `./configs/axle.yml` for example settings (ports, bootstrap peers, network id).

## Educational notes
- Difficulty uses a simple per-block adjustment bounded to ±1 leading zero bit step per block.
- Reward per block is recomputed as `reward = min(pool, pool / remaining_blocks)`, where
  `remaining_blocks ≈ max(1, (emission_end - now)/target_block_time)`. This is intentionally
  simple and easy to experiment with in classes.
- All tx types pay a 0.01 AXLE burn which is credited into the pool.

## Ticker and units
- Ticker: **AXLE**
- 1 AXLE = 10^8 nano-AXLE

## Test quickstart
```bash
ctest --test-dir build
```

## Roadmap for classes
- Swap storage to SQLite by implementing the same `IStateStore` interface using SQL.
- Experiment with different PoW loops in `miner.hpp` to test optimization ideas.
- Extend NFT with approvals or collection identifiers.
- Add anti-reorg and finalized checkpoints for lecture demos.
