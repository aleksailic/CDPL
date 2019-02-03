#define DEBUG_LINDA
#include "CDPL.h"

using namespace Concurrent;
using namespace Linda;
using namespace Testbed;

class Consumer : public Thread {
	void run() override {
		int tail, data;
		while (1) {
			std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 400 + 200));
			in("tail", &tail);
			out("tail", tail + 1);
			in("buffer", tail, &data);
			out("space");
		}
	}
};

class Producer : public Thread {
	void run() override {
		int head,data;
		while (1) {
			data = rand() % 50;
			std::this_thread::sleep_for(std::chrono::milliseconds(rand()%400+200));
			in("space");
			in("head", &head);
			out("head", head + 1);
			out("buffer", head, data);
		}
	}
};

constexpr int PCTEST_CAP = 5;


int main(){
    srand(random_seed);

    out("head", 0);
	out("tail", 0);
	for (int i = 0; i < PCTEST_CAP; i++)
		out("space");
	Producer producers[1];
	Consumer consumers[1];
	for (auto& p : producers)
		p.start();
	for (auto& c : consumers)
		c.start();

    producers[0].join();
    return 0;
}