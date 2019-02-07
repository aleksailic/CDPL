/*
	This example is part of Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

    ..............................................................................

    River crossing problem: Infinite amount of passengers from M groups want to
    cross the river. However, there is only one boat with maximum capacity C.
    There must always be the same number of passengers from each group in the boat
    before sailing.

    Solution using monitor
*/
#define DEBUG_BOAT
#include "CDPL.h"
#include <cstdlib>

using namespace Concurrent;
using namespace Testbed;
using namespace std::chrono_literals;

constexpr int num_of_groups  = 2;
constexpr int boat_capacity = 4;
constexpr int allowed_count = boat_capacity/num_of_groups; //if distributed that is allowed count of each
constexpr auto boat_travel_time = 5s; //for simulation

static_assert( boat_capacity % num_of_groups == 0 ,"Boat capacity must be divisible with number of groups");
static_assert( num_of_groups <= 4, "Number of groups must be less or equal to 4 for clarity purposes");
struct Passenger: public Thread{
    enum type_t{A,B,C,D};
    static std::atomic<int> next_id;
    type_t type;
    int id;
    void run() override;
    const char * map(type_t type){switch(type){case A: return "A";case B: return "B";case C: return "C";case D: return "D";};}
    Passenger(){
        id = next_id++;
        type = static_cast<type_t>(rand() % num_of_groups);
#ifdef DEBUG_BOAT
        fprintf(DEBUG_STREAM, "-- PASSENGER %s#%d created --\n",map(type),id);
#endif
    }
};
std::atomic<int> Passenger::next_id {0};

class Boat: public Monitorable{
    int count[num_of_groups] = {0};

    std::vector< Passenger::type_t > chosen_types;
    std::vector< const Passenger* > current_group;

    bool boat_here = true;

    cond boat_avail = cond_gen();
    cond boat_full = cond_gen();

    cond start_sail = cond_gen();
    cond end_sail = cond_gen();

    inline int max_per_type(){
        int divisor = chosen_types.size();
        while( boat_capacity % divisor != 0)
            divisor++;
        return boat_capacity / divisor;
    }
    inline int current_type_max(){
        int max_count = 0;
        for(int i=0;i<num_of_groups;i++)
            if(max_count < count[i])
                max_count = count[i];
        return max_count;
    }

    template <typename Container, typename Element>
    bool is_in(const Container& container, const Element& element){
        return std::find(std::begin(container), std::end(container), element) != std::end(container);
    }

    inline void board_allow(const Passenger& passenger){
        current_group.push_back(&passenger);
        count[passenger.type]++;
        print();
    }
    inline void board_prevent(const Passenger& passenger){
        boat_avail.wait();
        board(passenger); //not the best solution, but makes the code more readable
    }

    inline void print(){
        std::cout<<"BOAT: {";
        for(const Passenger* p: current_group)
			std::cout<< (p->type == Passenger::type_t::A ? "A" : "B")<<'#'<<p->id << " ";
		std::cout<<"}\n";
    }

    void reset(){
        for(int i=0;i<num_of_groups;i++) count[i]=0;
        chosen_types.clear();
        current_group.clear();
        boat_here = true;
        boat_avail.signalAll(); //code can be more efficient. left as an excercise, no need to complicate
    }
public:
    void board(const Passenger& passenger){
        while(!boat_here)
            boat_avail.wait();

        if(current_group.size()==0){ //boat is empty no need to test anything
            chosen_types.push_back(passenger.type);
            board_allow(passenger);
        }else if(current_group.size() < boat_capacity){
            if(is_in(chosen_types,passenger.type) && count[passenger.type] < max_per_type()){
                board_allow(passenger);
            }else if(!is_in(chosen_types,passenger.type)){ //need to check whether wr are allowed to make another group
                chosen_types.push_back(passenger.type);
                if(current_type_max() <= max_per_type()){
                    board_allow(passenger);
                }else{
                    chosen_types.pop_back();
                    board_prevent(passenger);
                }
            }else{
                board_prevent(passenger);
            }
        }else throw;

        if(current_group.size() != boat_capacity)
            boat_full.wait();
        else{
            boat_full.signalAll();
            boat_here = false;
            start_sail.signal();
        }
    }

    void sail(const Passenger& passenger){
        if(is_in(current_group,&passenger))
            end_sail.wait();
    }

    friend class Captain;
};
static monitor<Boat> boat;

class Captain: public Thread{
    void run()override{
        while(true){
            if(boat->boat_here)
                boat->start_sail.wait();
            std::cout<<"BOAT is sailing...\n";
            sleep_for(boat_travel_time);
            std::cout<<"BOAT is here!\n";
            boat->reset();
            boat->end_sail.signalAll();
        }
    }
public:
    ~Captain(){ join(); }
};

void Passenger::run(){
    boat->board(*this);
    boat->sail(*this);
}

int main(){
    srand(RANDOM_SEED);
    ThreadGenerator<Passenger> passenger_gen(1,2);
    Captain captain;

    captain.start();
    passenger_gen.start();
    return 0;
}
