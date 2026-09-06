#ifndef MKWII_GAMESPY_SESSIONS_H
#define MKWII_GAMESPY_SESSIONS_H

#include <cstdint>
#include <string>
#include <vector>

namespace mkwii {

struct OnlineSession {
	std::uint32_t session_id;
	std::string profile_id;
	std::string unique_nick;
	std::string game_name;
	std::string address;
	std::uint16_t port;
};

void upsert_online_session(const OnlineSession &session);
void remove_online_session(std::uint32_t session_id);
std::vector<OnlineSession> online_sessions_for_profiles(
	const std::vector<std::string> &profile_ids);

}  // namespace mkwii

#endif  // MKWII_GAMESPY_SESSIONS_H