#include "../catch.hpp"
#include <cdpl/concurrent/atomic.h>

#include <thread>
#include <chrono>

static class Simple {
public:
	explicit Simple(int a = 0, int b = 0) : a_(a), b_(b) {}

	void do_work(bool& started, bool& done, std::function<void(Simple& s)> on_complete = [](Simple&) {}) {
		started = true;
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		on_complete(*this);
		done = true;
	}

	int& a() {
		return a_;
	}
	int& b() {
		return b_;
	}
	int a(int value) {
		return a_ = value;
	}
private:
	int a_, b_;
};

TEST_CASE("Atomic initialization", "[atomic]") {
	cdpl::atomic<int> x = 5;
	cdpl::atomic<Simple> s(Simple{x}); // can convert cdpl::atomic<int> to int

	REQUIRE(x.get() == 5);
	REQUIRE(s.get().a() == 5);

	SECTION("Atomic can be copied around") {
		cdpl::atomic<int> w = x;
		REQUIRE(w.get() == 5);
	}
	SECTION("Atomic can be compared") {
		REQUIRE(x == 5);
		REQUIRE(5 == x);
	}
	SECTION("Calling through operator-> locks ") {
		bool done = false;
		bool started = false;

		std::thread t{ [&s, &done, &started] {
			s->do_work(started, done, [](Simple& self) {
				self.a(6);
			});
			s.notify_all();
		} };

		while (!started); 
		std::chrono::time_point start_time = std::chrono::high_resolution_clock::now();

		SECTION("Wait for mutex to access again through ->") {
			// try getting something
			REQUIRE(s->a() == 6);
			std::chrono::time_point end_time = std::chrono::high_resolution_clock::now();
			// if timestampits less than work time than object was not locked properly
			auto diff = end_time - start_time;
			REQUIRE(diff >= std::chrono::milliseconds(200));
		}

		SECTION("But calling get doesn't care") {
			// try getting something
			REQUIRE(s.get().a() == 5);
			std::chrono::time_point end_time = std::chrono::high_resolution_clock::now();
			auto diff = end_time - start_time;
			REQUIRE(diff < std::chrono::milliseconds(200));

			SECTION("Calling wait hangs until value changes") {
				s.wait();
				REQUIRE(s.get().a() == 6);
				std::chrono::time_point end_time = std::chrono::high_resolution_clock::now();
				auto diff = end_time - start_time;
				REQUIRE(diff >= std::chrono::milliseconds(200));
			}
		}

		SECTION("get_copy waits for mutex release") {
			// try getting something
			REQUIRE(s.get_copy().a() == 6);
			std::chrono::time_point end_time = std::chrono::high_resolution_clock::now();
			// if timestampits less than work time than object was not locked properly
			auto diff = end_time - start_time;
			REQUIRE(diff >= std::chrono::milliseconds(200));
		}

		if(t.joinable())
			t.join();
		
		
	}
}
