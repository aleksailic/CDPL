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

#define DEBUG

#include <cdpl.h>
#include <cstdlib>
#include <iostream>
#include <list>

constexpr auto N = 5;

cdpl::sem ticket(N-1, "ticket");
cdpl::mutex forks[N];

using namespace cdpl::utils;

class Philosopher: public cdpl::thread{
	int id;
	void think(){
		std::cout << lock << string_format("Philosopher(%d) is thinking\n", id) << unlock;
		sleep_for(std::chrono::seconds(rand() % 4));
		std::cout << lock << string_format("Philosopher(%d) finished thinking\n", id) << unlock;
	}
	void eat(){
		std::cout << lock << string_format("Philosopher(%d) is eating\n", id) << unlock;
		sleep_for(std::chrono::seconds(rand() % 4));
		std::cout << lock << string_format("Philosopher(%d) finished eating\n", id) << unlock;
	}
public:	
	Philosopher(int id) : id(id), cdpl::thread(string_format("philosopher #%d", id)) { }
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
	cdpl::log::set_level(cdpl::log::type::all);
	cdpl::log::info("philosophers", "N = %d", N);
	cdpl::log::info("philosophers", "ticket = %d", N - 1);

	srand(cdpl::testbed::random_seed);

	std::list<Philosopher> philosophers;
	for(int i=0;i<N;i++){
		philosophers.emplace_back(i);
		philosophers.back().start();
	}
}
