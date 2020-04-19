#include "../catch.hpp"
#include <cdpl/concurrent/thread.h>

#include <chrono>

TEST_CASE("Threads are created", "[thread]") {
	class worker : public cdpl::thread {	
	public:
		void run() override {
			cdpl::thread::sleep_for(std::chrono::milliseconds(200));
			done_ = true;
		}
		bool done() { return done_; }
	private:
		bool done_ = false;
	};

	worker a;
	a.start();
	REQUIRE(a.done() == false);
	a.join();
	REQUIRE(a.done() == true);
}