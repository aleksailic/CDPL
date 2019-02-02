#include "kdp.h"
#include <functional>

using namespace MPI;
using namespace Testbed;

struct BufferTest : public Test {
	void simulate() override {}
	grade_t grade() override {
		return run_tests([] {
			MonitorMessageBox<int> buffer;
			buffer->put(6);
			buffer->put(8);
			buffer->put(4);

			if (buffer->get() != 6)
				return false;
			if (buffer->get() != 8)
				return false;
			if (buffer->get() != 4)
				return false;

			std::thread([&buffer] {
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				buffer->put(8);
				std::this_thread::sleep_for(std::chrono::milliseconds(700));
				buffer->put(8);
			}).detach();

			std::thread([&buffer] {
				std::this_thread::sleep_for(std::chrono::milliseconds(300));
				buffer->get();
				std::this_thread::sleep_for(std::chrono::milliseconds(670));
				buffer->put(8);
			}).detach();

			if (buffer->get() != 8)
				return false;
			if (buffer->get() != 8)
				return false;
			return true;
		});
	}
};

int main(){
	auto test = BufferTest();
	std::cout << test.grade();

	return 0;
}