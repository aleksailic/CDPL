/*
	This example is part of Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "CDPL.h"
#include <ctime>
#include <cstdlib>

#define DEBUG_ALARM

using namespace Concurrent;
using namespace Testbed;

constexpr auto tick_interval = std::chrono::seconds(1);

class Alarm:public Monitorable{
	uint current_ticks = 0;
	cond wakeup = cond_gen();

	void onTick(){
		current_ticks++;
#ifdef DEBUG_ALARM
		//note: printing %d for minrank instead of %u. -1 is much prettier sight than MAX_UINT
		fprintf(DEBUG_STREAM, "-- tick: %d, minrank: %d--\n",current_ticks, wakeup.minrank());
#endif
		if(!wakeup.empty() && wakeup.minrank() <= current_ticks)
			wakeup.signal();
	}
	struct Ticker: public Thread{ 
		void run();
	};
	Ticker* ticker = nullptr;
public:
	Alarm(Mutex& mutex):Monitorable(mutex){}
	~Alarm(){delete ticker;}

	void wake_in(uint ticks_from_now){
		wakeup.wait(current_ticks + ticks_from_now);
		if(!wakeup.empty() && wakeup.minrank()<=current_ticks)
			wakeup.signal();
	}

	void start(){
		if(ticker == nullptr){
			ticker = new Ticker();
			ticker->start();
		}
	}
};

static monitor<Alarm> alarm;

void Alarm::Ticker::run(){
	while(true){
		sleep_for(tick_interval);
		alarm->onTick();
	}
}
struct Worker:public Thread{
	static std::atomic<uint> next_id;
	uint id;
	void run() override{
		id = next_id++;
		uint wake_in  = rand() % 10 + 1;
		fprintf(stdout, "worker[#%d]: waking in %d ticks\n", id, wake_in);
		alarm->wake_in(wake_in);
		fprintf(stdout, "worker[#%d]: awake!\n", id);
	}
};
std::atomic<uint> Worker::next_id {0};

using namespace std;
int main(){
	srand(random_seed);

	ThreadGenerator<Worker> worker_generator(3,7);
	worker_generator.start();

	alarm->start();
	
	return 0;
}