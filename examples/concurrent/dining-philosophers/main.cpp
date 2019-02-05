/*
	This example is part of Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

	..............................................................................

	Dining philosophers problem: N philosophers sit at a round table. Forks are
	placed between each pair of adjacent philosophers. Each philosopher must
	alternately think and eat. However, a philosopher can only eat when they have
	both left and right forks. Each fork can be held by only one philosopher.
	After an individual philosopher finishes eating, they need to put down both
	forks so that the forks become available to others. Philosopher cannot start
	eating before getting both forks.

	Solution using semaphores and ticket algorithm
*/

#define DEUBG_PHILOSOPHERS
#include "CDPL.h"
#include <cstdlib>

using namespace Concurrent;
using namespace Testbed;

#define N 5

sem_t ticket {N-1};
mutex_t forks[N];

class Philosopher: public Thread{
	int id;
	inline void think(){
		#ifdef DEUBG_PHILOSOPHERS
			fprintf(DEBUG_STREAM, "Philosopher(%d) is thinking\n",id);
		#endif
		std::this_thread::sleep_for(std::chrono::seconds(rand()%4));
		#ifdef DEUBG_PHILOSOPHERS
			fprintf(DEBUG_STREAM, "Philosopher(%d) finished thinking\n",id);
		#endif
	}
	inline void eat(){
		#ifdef DEUBG_PHILOSOPHERS
			fprintf(DEBUG_STREAM, "Philosopher(%d) is eating\n",id);
		#endif
		std::this_thread::sleep_for(std::chrono::seconds(rand()%4));
		#ifdef DEUBG_PHILOSOPHERS
			fprintf(DEBUG_STREAM, "Philosopher(%d) finished eating\n",id);
		#endif
	}
public:	
	Philosopher(int id):id(id){}
	void run() override{
		int left=id, right=(id+1)%N;
		while(1){
			think();
			ticket.wait();
			forks[left].lock();
			forks[right].lock();
			eat();
			forks[right].unlock();
			forks[left].unlock();
			ticket.signal();
		}
	}
	~Philosopher(){
		join();
	}
};

int main(){
	srand(RANDOM_SEED);
	#ifdef DEUBG_PHILOSOPHERS
		fprintf(DEBUG_STREAM, "-- init: %d philosophers --\n",N);
	#endif

	std::list<Philosopher> philosophers;
	for(int i=0;i<N;i++){
		philosophers.push_back(Philosopher(i));
		philosophers.back().start();
	}
}
