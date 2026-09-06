#include "mkwii/gamespy_sessions.h"

#include <cassert>
#include <vector>

int main() {
	mkwii::upsert_online_session({1, "101", "Alice", "mariokartwii", "192.0.2.1", 27900});
	mkwii::upsert_online_session({2, "202", "Bob", "mariokartwii", "192.0.2.2", 27901});

	const std::vector<mkwii::OnlineSession> both =
		mkwii::online_sessions_for_profiles({"202", "101", "missing"});
	assert(both.size() == 2);
	assert(both[0].profile_id == "202");
	assert(both[1].profile_id == "101");

	mkwii::remove_online_session(1);
	const std::vector<mkwii::OnlineSession> remaining =
		mkwii::online_sessions_for_profiles({"101", "202"});
	assert(remaining.size() == 1);
	assert(remaining[0].unique_nick == "Bob");
	return 0;
}