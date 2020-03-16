#define DEBUG_THREAD

#include "../catch.hpp"
#include <cdpl/concurrent/thread.h>


TEST_CASE("Threads are created", "[thread]") {
	using thread = cdpl::concurrent::thread;

	bool value_changed = false;

	class worker : public thread {		
		bool* value;
	public:
		worker(bool* variable) : value(variable), thread() {}
		void run() override {
			DEBUG_WRITE("thread", "value changed");
			*value = true;
		}
	};

	worker a{&value_changed};
	a.start();
	a.join();

	REQUIRE(value_changed == true);
}