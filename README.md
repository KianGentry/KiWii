# KiWii

A self-hosted Mario Kart Wii online service.

Requirements:

- C++20 compiler (g++)
- CMake 3.20 or newer (cmake)
- Python 3 (python3)

## Docker

```sh
docker compose up -d
```

The container currently provides health, the GameSpy QR availability response,
and the plain HTTP NAS connectivity response. See TODO for what is unfinished.

## From source

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

## Misc info

Health is at `http://127.0.0.1:8080/` with the example config. Curl to get a quick status report.
