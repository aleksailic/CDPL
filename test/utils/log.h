#include <cdpl/utils/log.h>

TEST_CASE("Log works") {
	using namespace cdpl::log;

	cdpl::log::info("log", "Running some tests!");
	cdpl::log::error("log", "This is how error looks!");
	cdpl::log::debug("log", "This is how debug looks!");
	cdpl::log::warn("log", "This is how warning looks!");
	_log(type::id::debug, "test", "My custom %.2f format %s", 0.0456f, "test");
}