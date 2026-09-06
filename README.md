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

The container currently provides health, GameSpy QR registration, the initial
GameSpy NATNEG acknowledgement, the GameSpy player-search response, and the
plain HTTP NAS connectivity response. See TODO for what is unfinished.

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

## Internet deployment

Set `MKWII_SERVER_NAME` and `MKWII_ADVERTISED_ADDRESS` in `.env` before starting
Compose. `MKWII_ADVERTISED_ADDRESS` must be the public IPv4 address or hostname
that resolves to the KiWii host, not the container address.

Forward these currently implemented listeners from the public Internet:

| Protocol | Default port | Service |
| --- | ---: | --- |
| TCP | 53 | DNS |
| UDP | 53 | DNS |
| TCP | 80 | NAS |
| UDP | 27900 | GameSpy QR |
| UDP | 27901 | NATNEG |
| TCP | 28910 | GameSpy browser connection |
| TCP | 29900 | GameSpy profile |
| TCP | 29901 | Player Search |
| TCP | 22000 | Relay TLS |

The health listener on TCP `8080` is intended for local or private monitoring.
The browser UDP endpoint and relay UDP endpoint are not published because those
protocol handlers are not implemented yet.
