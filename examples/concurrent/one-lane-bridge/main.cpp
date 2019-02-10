/*
	This example is part of Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

	..............................................................................

	One lane bridge problem: Bridge has only one lane. Cars are coming from NORTH
	and SOUTH. There can only be MAX_CARS on the bridge at the same time. Try to
	minimize starvation.

	Solution using monitor
*/

#define DEBUG_COND
#define DEBUG_BRIDGE

#include "CDPL.h"
#include <cstdlib>

using namespace Concurrent;
using namespace Testbed;
using namespace Utils;

typedef int dir_t;
enum {SOUTH=0,NORTH};
dir_t opp(dir_t dir){return (dir == SOUTH ? NORTH : SOUTH);}

class Bridge;
class Car: public Thread{
	static std::atomic<int> next_id;
	void run() override;

	const dir_t	dir = 0;
	const int id;
	std::string name;
public:
	Car(dir_t direction): id(next_id++), dir(direction){
		name = string_format("CAR[%s#%d]",(dir == SOUTH ? "S" : "N"),id);
		Thread::set_name(name.c_str());
	};

	friend Bridge;
};
std::atomic<int> Car::next_id {0};


class Bridge:public Monitorable{
	static constexpr int THRESH = 5;
	static constexpr int MAX_CARS = 2;

	cond enter [2] = { cond_gen("enter[S]"), cond_gen("enter[N]") };
	cond car_exit = cond_gen("car_exit");

	dir_t current_dir = NORTH;
	uint current_count = 0;
	uint cars_on_bridge = 0;

	bool flip_schedueled = false;

	cond bridge_empty = cond_gen("bridge_empty");

public:
	void pass(Car& car){
		//rather lengthy if, skecth out logic on paper it is more easier than to explain like this
		if( !enter[opp(car.dir)].empty() && ((current_dir==car.dir && flip_schedueled) || ((current_dir==opp(car.dir) && !flip_schedueled))) ){
			enter[car.dir].wait(); //wait for your turn
		}
		if(cars_on_bridge>0 && current_dir== opp(car.dir)){ //wait for opposite turn to fully complete
			flip_schedueled=true; //i've arrived before others from other side were waiting let me go!
			bridge_empty.wait();
			//what if new guy from opp dir arrived JIT before bridge empties? q: who shall pass? a: depends on scheduler. inconclusive in this example
		}
		//so you are allowed to enter but max car is stopping you, okay just wait for that
		if(cars_on_bridge==MAX_CARS)
			car_exit.wait();

		//CAR IS ALLOWED TO PASS

		current_dir = car.dir; //car can pass if there is no one on the other side so update this var
		//only keep count if cars are waiting from other side
		if( !enter[opp(car.dir)].empty()){
			current_count++;
		}
		cars_on_bridge++;
		//signal next car to pass depending on threshold
		if(current_count<THRESH){
			enter[car.dir].signal();
		}else{
			flip_schedueled = true;
			enter[opp(car.dir)].signal();
		}
	}
	void exit(Car& car){
		cars_on_bridge--;
		car_exit.signal();
		if(cars_on_bridge==0){
			bridge_empty.signal();
			if(flip_schedueled){
				current_dir	  = opp(current_dir);
				current_count = 0;
				flip_schedueled = false;
			}
		}
	}
};

static monitor<Bridge> mon;

void Car::run(){
	std::cout << lock << name << colorize(" created", TC::YELLOW) << std::endl << unlock;
	//CAR TRYING TO PASS
	mon->pass(*this);

	std::cout << lock << name << " is " << colorize("passing",TC::RED, TS::BOLD) << std::endl << unlock;
	//CAR IS NOW DRIVING THROUGH THE BRIDGE
	sleep_for(std::chrono::seconds(rand() % 4 + 3));

	std::cout << lock << name << colorize(" exiting", TC::GREEN) << std::endl << unlock;
	//CAR EXIT BRIDGE
	mon->exit(*this);
}

int main(int argc, char** argv){
	srand(random_seed);
	std::vector<ThreadGenerator<Car>> generators = { {1s, 5s, NORTH}, {1s, 5s, SOUTH} };
	
	for(auto& generator: generators)
		generator.start();
	
	return 0;
}
