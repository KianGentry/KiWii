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

## Docker

```sh
docker compose up -d
```

The initial container only provides the health endpoint. The DNS and game ports are reserved in the compose file so the deployment shape is established before protocol stuff is added.
