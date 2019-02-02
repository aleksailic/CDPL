#include "kdp.h"
#include <cstdlib>
#include <unistd.h>

#include <atomic>

using namespace Concurrent;
using namespace Testbed;

typedef int dir_t;
enum {SOUTH=0,NORTH};
dir_t opp(dir_t dir){return (dir == SOUTH ? NORTH : SOUTH);}

std::atomic<int> next_id(0);

class Bridge;
class Car: public Thread{
	void run() override;
	const dir_t		dir = 0;
	const int		 id;
public:
	Car():id(++next_id){
		dir_t* my_dir = const_cast<dir_t*>(&dir);
		*my_dir = rand() % 2;
	};

	friend Bridge;
};


class Bridge:public Monitorable{
	static constexpr int THRESH = 5;
	static constexpr int MAX_CARS = 2;

	cond enter [2] = { cond_gen(), cond_gen() };
	cond car_exit = cond_gen();

	dir_t current_dir = NORTH;
	uint current_count = 0;
	uint cars_on_bridge = 0;

	bool flip_schedueled = false;

	cond bridge_empty = cond_gen();

public:
	Bridge(Mutex& mutex):Monitorable(mutex){};
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
mutex_t print_mutex;

void Car::run(){
	print_mutex.lock();
	std::cout<< "CAR[" << (dir == SOUTH ? "S" : "N") << '#' << id << "] CREATED\n";
	print_mutex.unlock();
	//CAR TRYING TO PASS
	mon->pass(*this);

	print_mutex.lock();
	std::cout<< "CAR[" << (dir == SOUTH ? "S" : "N") << '#' << id << "] IS PASSING\n";
	print_mutex.unlock();
	//CAR IS NOW DRIVING THROUGH THE BRIDGE
	sleep(rand() % 4 + 3);

	print_mutex.lock();
	std::cout<< "CAR[" << (dir == SOUTH ? "S" : "N") << '#' << id << "] EXITING\n";
	print_mutex.unlock();
	//CAR EXIT BRIDGE
	mon->exit(*this);
}

int main(int argc, char** argv){
	srand(random_seed);
	
	std::vector< ThreadGenerator<Car> > many_car_generators(1,ThreadGenerator<Car>(1,3));
	for(auto& gen: many_car_generators)
		gen.start();
	return 0;	
}