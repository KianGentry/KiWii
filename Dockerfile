FROM gcc:14 AS build
WORKDIR /src
RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    build-essential \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . .
RUN cmake -S . -B /tmp/kiwii-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
    && cmake --build /tmp/kiwii-build --parallel

FROM debian:trixie-slim
RUN apt-get update \
    && apt-get install -y --no-install-recommends libcap2-bin libssl3 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --create-home --uid 10001 mkwii
COPY --from=build /tmp/kiwii-build/mkwii-server /usr/local/bin/mkwii-server
RUN setcap 'cap_net_bind_service=+ep' /usr/local/bin/mkwii-server
USER mkwii
ENTRYPOINT ["/usr/local/bin/mkwii-server"]
