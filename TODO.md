# TODO / Roadmap

## Immediate

- [ ] Remove the remaining legacy duplicate implementations from `src/server.cpp`
- [ ] Replace deprecated OpenSSL RSA generation calls with EVP key-generation APIs
- [ ] Add focused DNS protocol tests
- [ ] Add focused profile idle-session/keepalive tests
- [ ] Add focused Sake SOAP response tests for every supported action
- [ ] Add a recorded-packet integration test for the complete login-to-menu flow
- [ ] Verify the current Docker image after the source-file split
- [ ] Update the startup log to report all configured listeners consistently

## Sake FriendInfo

- [x] Add an in-memory or SQLite-backed `g1687_FriendInfo` store
- [x] Parse Sake SOAP actions instead of matching action names with substrings
- [ ] Validate Sake `gameid`, `tableid`, `secretKey`, and `loginTicket`
- [ ] Map Sake login tickets to authenticated profile IDs
- [x] Persist the submitted binary `info` value on `CreateRecord`
- [x] Generate stable record IDs instead of returning hard-coded `recordid=1`
- [x] Return the client’s own records from `GetMyRecords`
- [ ] Add support for `GetSpecificRecords` if the client requests it
- [ ] Add support for `GetRecordCount` if the client requests it
- [ ] Add support for `UpdateRecord` if the client requests it
- [ ] Keep FriendInfo state across server restarts when SQLite storage is enabled

## Matchmaking Services

- [ ] Capture and decode the Player Search request/response sequence in detail
- [x] Define the online-player/session model
- [x] Track QR-registered clients by profile, game, public endpoint, and session
- [x] Remove registered clients on `statechanged=2`
- [x] Return only real active clients from Player Search
- [x] Keep the empty-player response correct when nobody is online
- [ ] Add a configurable synthetic opponent for one-client protocol testing
- [ ] Keep synthetic matchmaking disabled by default
- [ ] Implement the GameSpy browser request parser on TCP `28910`
- [ ] Implement the minimal empty server-list response
- [ ] Add the current client/server to the browser list when appropriate
- [ ] Add encrypted browser responses if the PAL client requires them

## NATNEG And Session Transport

- [ ] Capture the first real UDP `27901` NATNEG sequence
- [x] Implement NATNEG session tracking by session ID and client index
- [x] Implement `NN_INIT` and `NN_INITACK` using captured packets
- [x] Implement the first observed `NN_CONNECT` exchange
- [ ] Implement NAT type and endpoint fields from real captures
- [ ] Add NATNEG timeout and session cleanup
- [ ] Determine whether this Mario Kart Wii revision uses NATNEG or the relay path
- [ ] Support both raw/no-SSL and TLS relay handshakes
- [ ] Detect TLS by the initial record bytes instead of assuming TLS
- [ ] Keep the no-SSL patched-client path working
- [ ] Parse the decrypted relay/session payloads
- [ ] Implement the relay/session acknowledgement required by the client
- [ ] Support UDP/QUIC session transport if the client requires it
- [ ] Replace LAN broadcast discovery with unicast server-side rendezvous
- [ ] Advertise KiWii-controlled public endpoints instead of LAN addresses
- [ ] Add peer/session routing for two clients
- [ ] Add relay forwarding when direct peer-to-peer traffic is unavailable

## Two-Client Local Validation

- [ ] Run two clients against the same KiWii instance on the LAN
- [ ] Confirm both clients authenticate independently
- [ ] Confirm both clients appear in the server-side session registry
- [ ] Confirm both clients can discover one another
- [ ] Confirm both clients complete browser/NATNEG or relay setup
- [ ] Confirm both clients reach character select together
- [ ] Confirm both clients exchange race-start traffic
- [ ] Confirm one client disconnect does not destroy the other session
- [ ] Add a repeatable two-client capture procedure
- [ ] Add packet fixtures for the two-client handshake

## Internet Deployment

- [ ] Configure `MKWII_ADVERTISED_ADDRESS` for the public address or hostname
- [ ] Document required router forwarding for TCP and UDP ports
- [ ] Forward DNS port `53` TCP/UDP to KiWii
- [ ] Forward NAS TCP `80`
- [ ] Forward QR UDP `27900`
- [ ] Forward NATNEG UDP `27901`
- [ ] Forward browser TCP/UDP `28910` as required
- [ ] Forward profile TCP `29900`
- [ ] Forward Player Search TCP `29901`
- [ ] Forward relay TCP/UDP `22000`
- [ ] Verify the host is not behind CGNAT
- [ ] Add dynamic DNS guidance for changing public IPs
- [ ] Add host firewall rules and verification commands
- [ ] Ensure Docker publishes every required protocol/port
- [ ] Verify DNS answers from an external network
- [ ] Verify each TCP service from an external network
- [ ] Verify UDP QR/NATNEG/relay reachability externally
- [ ] Capture a two-client Internet session
- [ ] Validate direct peer connectivity across different networks
- [ ] Validate relay fallback when direct connectivity fails

## Reliability And Security

- [ ] Replace detached/unbounded worker growth with a managed worker strategy
- [ ] Add bounded request sizes to every protocol handler
- [ ] Add malformed-packet tests for DNS, QR, NATNEG, browser, and Sake
- [ ] Add per-session expiration and cleanup
- [ ] Avoid logging credentials, tokens, or full private payloads by default
- [ ] Add configurable log levels
- [ ] Make TLS certificate/key configuration optional and documented
- [ ] Persist the relay certificate when stable identity is needed
- [ ] Run sanitizers in CI
- [ ] Add dependency and container vulnerability scanning
- [ ] Add graceful shutdown coverage for active profile, Sake, and relay sessions

## Documentation And Operations

- [ ] Document native build and test commands
- [ ] Document Docker Compose setup
- [ ] Document `.env` configuration
- [ ] Document Wii DNS configuration
- [ ] Document Dolphin network configuration
- [ ] Document the no-SSL patch as optional compatibility tooling
- [ ] Document LAN-only testing
- [ ] Document Internet deployment and port forwarding
- [ ] Document troubleshooting with `ss`, `tcpdump`, `tshark`, and logs
- [ ] Add a protocol architecture overview
- [ ] Add a capture-driven development guide
- [ ] Add a release checklist

## Later Features

- [ ] Web status UI
- [ ] Public server list
- [ ] Multiple rooms and matchmaking policies
- [ ] Persistent player accounts and friend lists
- [ ] Administratively configurable server visibility
- [ ] Support additional Mario Kart Wii revisions
- [ ] Support additional games only after the shared services stabilize