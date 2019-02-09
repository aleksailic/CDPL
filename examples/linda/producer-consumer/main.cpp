/*
	This example is part of Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

	..............................................................................

	Producer-consumer problem: 2 processes coexist, the producer and the consumer,
	who share a common, fixed-size buffer used as a queue. The producer's job is
	to generate data, put it into the buffer, and start again. At the same time,
	the consumer is consuming the data (i.e., removing it from the buffer),
	one piece at a time. Make sure that the producer won't try to add data into
	the buffer if it's full and that the consumer won't try to remove data from an
	empty buffer.
*/

#define DEBUG_LINDA
#include "CDPL.h"

using namespace Concurrent;
using namespace Linda;
using namespace Testbed;

class Consumer : public Thread {
	void run() override {
		int tail, data;
		while (1) {
			sleep_for(std::chrono::milliseconds(rand() % 400 + 200));
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
			sleep_for(std::chrono::milliseconds(rand()%400+200));
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
