#include "../catch.hpp"
#include <cdpl/concurrent/monitor.h>

#include <chrono>
/*
TEST_CASE("Monitors are created", "[monitor]") {
	template <class T, int Capacity = 10>
	class buffer : public cdpl::monitorable {
	public:
		T get() {
			while (empty())
				item_available.wait();
		}
		void put(const T& elem) {

		}
		bool empty() const {
			return size == 0;
		}
	private:
		auto item_available = condition_binder();
		auto space_available = condition_binder();
		std::array<T, Capacity> data_;
		size_t size = 0;
	};

	class producer : public cdpl::thread {
	public:
		producer(cdpl::monitor<buffer>& buffer) {

		}
	private:
		cdpl::monitor<buffer>& buf_;
	};

	class consumer : public cdpl::thread {

	};


	class barrier : public cdpl::monitorable {
	public:
		bool done() { return done_; }
	private:
		bool done_ = false;
	};

	cdpl::monitor<barrier> b{};
	REQUIRE(b->done() == false);
}
*/