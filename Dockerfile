FROM gcc:14 AS build
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
    && cmake --build build --parallel

FROM debian:bookworm-slim
RUN useradd --system --create-home --uid 10001 mkwii
COPY --from=build /src/build/mkwii-server /usr/local/bin/mkwii-server
USER mkwii
ENTRYPOINT ["/usr/local/bin/mkwii-server"]
