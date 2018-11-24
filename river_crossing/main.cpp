#define NODEBUG

#include "req.h"
#include <stdlib.h> 

constexpr int num_of_groups  = 2;
constexpr int boat_capacity = 4;
constexpr int allowed_count = boat_capacity/num_of_groups; //if distributed that is allowed count of each

//for simulation
constexpr int boat_travel_time = 5; //in seconds


struct Passenger: public Thread{
	enum t_id {A=1,B};
	static int last_id;
	int id;
	t_id type ;
	void run() override;

	Passenger(){
		id=last_id++;
		type = static_cast<t_id>(rand() % 2 + 1);
	}
};
int Passenger::last_id = 0;

class Captain;
class Boat: public Monitorable{
	enum s_id {ALL,DISTRIBUTE,NYD}; //whether to accept all from one group, to distribute or not yet determined
	int group_count[num_of_groups]={0};

	Passenger* current_group[boat_capacity];
	int current_group_count = 0;

	s_id state = NYD;
	Passenger::t_id chosen_group = static_cast<Passenger::t_id>(0); //group is not yet chosen

	bool boat_available = true;
	cond boat_return = cond_gen();

	Semaphore ready_to_sail;

	void print(){
		std::cout<<"BOAT: {";
		for(int i=0;i<current_group_count;i++)
			std::cout<< (current_group[i]->type == Passenger::t_id::A ? "A" : "B")<<'#'<<current_group[i]->id << ' ';
		std::cout<<"}\n";
	}

	friend Captain; //captain can access nitsy bitsy elements
public:
	Boat(Mutex& mutex):Monitorable(mutex),ready_to_sail(0){}

	bool insert(Passenger& passenger){//boat is not here :(, need to wait
		if(!boat_available) return false;
		switch(state){
			case NYD:
				if(!chosen_group){
					group_count[chosen_group = passenger.type]++;
					current_group [current_group_count++] = &passenger;
				}else if(chosen_group == passenger.type){
					state = ALL;
					return insert(passenger);
				}else{
					state = DISTRIBUTE;
					return insert(passenger);
				}
				break;
			case DISTRIBUTE:
				if(group_count[passenger.type]==allowed_count) // whoops, max count reached, no go for you
					return false;
				else{
					group_count[passenger.type]++; //okay you can go...
					current_group[current_group_count++] = &passenger; //add passenger to the current group
				}
				break;
			case ALL:
				if(passenger.type != chosen_group) // whoops, wrong group wait my friend
					return false;
				else{
					group_count[passenger.type]++; //okay you can go...
					current_group[current_group_count++] = &passenger; //add passenger to the current group
				}
				break;
		}
		print();
		if(current_group_count == boat_capacity){
			std::cout<<"BOAT IS SAILING :) \n";
			boat_available = false;
			ready_to_sail.signal(); //time_to_sail
		}
		return true; //boy on board
	}

	void waitBoat(){
		boat_return.wait();	
	}
};

class Captain:public Thread{
	Boat& my_boat;
public:
	Captain(Boat& boat):my_boat(boat){}
	void run() override{
		while(true){
			my_boat.ready_to_sail.wait();	
			sleep(boat_travel_time);

			for(int& counter: my_boat.group_count)
				counter=0;
			my_boat.current_group_count=0;
			my_boat.boat_available=true;

			my_boat.boat_return.signalAll(); // vratija se sime!!
			std::cout<<"VRATIJA SE SIME!!! \n";
		}
	}
};


monitor<Boat>* mon;

void Passenger::run(){
	std::cout<< "PASSENGER " << (type == A ? "A" : "B") << "#"<<id<<" CREATED" << std::endl;
	while(!(*mon)->insert(*this))
		(*mon)->waitBoat();
}


int main(){
	srand(random_seed);
	mon = new monitor<Boat>;

	Captain captain(mon->demonitorize()); //captain gets his boat
	captain.start();
	ThreadGenerator<Passenger> tg(2,3); //2 and 3 sec diff
	tg.start();
}