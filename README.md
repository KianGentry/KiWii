# KiWii

A self-hosted Mario Kart Wii online service.

Requirements:

- C++20 compiler (g++)
- CMake 3.20 or newer (cmake)
- Python 3 (python3)

Build and test:

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Run service:

```sh
cmake --build build
set -a; . ./.env; set +a
./build/mkwii-server
```

Health is at `http://127.0.0.1:8080/` with the example config.

The prototype currently provides the GameSpy QR availability response on UDP
port `27900` and the AltWFC-compatible plain HTTP connectivity response on
port `80`. The client still attempts the connectivity check over HTTPS on port
`443`; a no-SSL client patch is therefore required before the NAS endpoint can
be used by Dolphin.

## Docker

```sh
docker compose up -d
```

The container currently provides health, the GameSpy QR availability response,
and the plain HTTP NAS connectivity response. DNS, NAT negotiation, and game
session services are not implemented yet.
