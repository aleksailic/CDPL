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

	Solution using Linda and ticket algorithm
*/

#define DEBUG_LINDA
#define DEUBG_PHILOSOPHERS

#include "cdpl.h"

#include <cstdlib>

using namespace cdpl::linda;
constexpr int N = 5;

class Philosopher: public Active<void*>{
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
	void* run() override{
		int left=id, right=(id+1)%N;
		while(1){
			think();
			in("ticket");
			in("fork",left);
			in("fork",right);
			eat();
			out("fork",left);
			out("fork",right);
			out("ticket");
		}
		return nullptr;
	}
};

int main(){
	srand(RANDOM_SEED);
	#ifdef DEUBG_PHILOSOPHERS
		fprintf(DEBUG_STREAM, "-- init: %d philosophers --\n",N);
	#endif
	for(int i=0;i<N;i++){
		out("fork",i);
		eval(Philosopher(i));
		if(i<N-1)out("ticket");
	}
	in("finish");
}
