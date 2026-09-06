#include "mkwii/gamespy_sessions.h"

#include <mutex>
#include <chrono>
#include <unordered_map>

namespace mkwii {
namespace {

std::unordered_map<std::uint32_t, OnlineSession> online_sessions;
std::unordered_map<std::uint32_t, std::chrono::steady_clock::time_point> last_seen;
std::mutex online_sessions_mutex;

}  // namespace

void upsert_online_session(const OnlineSession &session) {
	std::lock_guard<std::mutex> lock(online_sessions_mutex);
	online_sessions[session.session_id] = session;
	last_seen[session.session_id] = std::chrono::steady_clock::now();
}

void remove_online_session(std::uint32_t session_id) {
	std::lock_guard<std::mutex> lock(online_sessions_mutex);
	online_sessions.erase(session_id);
	last_seen.erase(session_id);
}

void prune_online_sessions(std::chrono::seconds max_age) {
	const auto cutoff = std::chrono::steady_clock::now() - max_age;
	std::lock_guard<std::mutex> lock(online_sessions_mutex);
	for (auto iterator = last_seen.begin(); iterator != last_seen.end();) {
		if (iterator->second <= cutoff) {
			online_sessions.erase(iterator->first);
			iterator = last_seen.erase(iterator);
		} else {
			++iterator;
		}
	}
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