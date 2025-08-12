FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends     build-essential cmake git pkg-config libsodium-dev ca-certificates  && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j

EXPOSE 9735 9736
CMD ["./build/axle", "start", "--datadir", "/app/data", "--p2p", "0.0.0.0:9735", "--rpc", "0.0.0.0:9736"]
