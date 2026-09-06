#include "mkwii/gamespy_sessions.h"

#include <mutex>
#include <unordered_map>

namespace mkwii {
namespace {

std::unordered_map<std::uint32_t, OnlineSession> online_sessions;
std::mutex online_sessions_mutex;

}  // namespace

void upsert_online_session(const OnlineSession &session) {
	std::lock_guard<std::mutex> lock(online_sessions_mutex);
	online_sessions[session.session_id] = session;
}

void remove_online_session(std::uint32_t session_id) {
	std::lock_guard<std::mutex> lock(online_sessions_mutex);
	online_sessions.erase(session_id);
}

std::vector<OnlineSession> online_sessions_for_profiles(
	const std::vector<std::string> &profile_ids) {
	std::vector<OnlineSession> matches;
	std::lock_guard<std::mutex> lock(online_sessions_mutex);
	for (const std::string &profile_id : profile_ids) {
		for (const auto &[session_id, session] : online_sessions) {
			if (session.profile_id == profile_id) {
				matches.push_back(session);
			}
		}
	}
	return matches;
}

}  // namespace mkwii