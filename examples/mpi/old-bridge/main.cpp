/*
    Similar to one_lane_bridge problem, just needs to utilize MPI as means of communication
    and no optimization for direction flip on max car pass
*/
#include "kdp.h"
#include <cstdlib>

#define N_CARS 20
#define MAX_MASS 201

using namespace Concurrent;
using namespace MPI;
using namespace Testbed;

enum dir_t{NORTH,SOUTH};
enum op_t{WAIT,PASS,ENTER,EXIT};
enum err_t{ERR_INVALID_OP,ERR_CAR_OVERFLOW};

struct msg_t{
    uint id;
    op_t op;
    dir_t dir;
    uint mass;
};

class Car: public Thread{
    MonitorMessageBox<msg_t>& bridge_mbx;
    dir_t direction;
    uint id;
    uint mass;
public:
    static MonitorMessageBox<msg_t> mbx[N_CARS];
    static uint next_id;
    Car(MonitorMessageBox<msg_t>& bridge_mbx):bridge_mbx(bridge_mbx){
        id = next_id++;
        if(id == N_CARS)
            throw ERR_CAR_OVERFLOW;
        direction = static_cast<dir_t>(rand() % 2);
        mass = rand() % 70 + 30;

        std::cout<< "CAR[" << (direction == SOUTH ? "S" : "N") << '#' << id << "] CREATED"  << std::endl;
    }
    void run() override{
        bridge_mbx->put(msg_t{id,ENTER,direction,mass});
        msg_t msg{0,WAIT};
        while(msg.op == WAIT)
            msg = mbx[id]->get();

        std::cout<< "CAR[" << (direction == SOUTH ? "S" : "N") << '#' << id << "] IS PASSING" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(rand() % 4 + 3));

        bridge_mbx->put(msg_t{id,EXIT,direction,mass});
        std::cout<< "CAR[" << (direction == SOUTH ? "S" : "N") << '#' << id << "] EXITING" << std::endl;
    }
};
MonitorMessageBox<msg_t> Car::mbx[N_CARS];
uint Car::next_id=0;

class OldBridge: public Thread{
    MonitorMessageBox<msg_t>& mbx;
    dir_t current_dir = SOUTH;
    uint current_mass = 0;
    std::list<msg_t> wait_list;
public:
    OldBridge(MonitorMessageBox<msg_t>&mbx):mbx(mbx){}
    void run() override{
        while(true){
            msg_t msg = mbx->get();
            switch(msg.op){
                case EXIT:
                    current_mass -= msg.mass;
                    if(current_mass == 0 && wait_list.size() != 0) //all cars passed and other cars are waiting in oposite direction
                        current_dir = current_dir==NORTH ? SOUTH : NORTH;
                    for(auto i = wait_list.begin(); i!= wait_list.end();){
                        if(i->dir == current_dir && current_mass+i->mass < MAX_MASS){
                            Car::mbx[i->id]->put(msg_t{0,PASS,(dir_t)0,0}); //let him pass
                            current_mass+=i->mass;
                            i = wait_list.erase(i);
                        }else
                            i++;
                    }
                    break;
                case ENTER:
                    if( msg.dir == current_dir && current_mass+msg.mass < MAX_MASS){
                        current_mass += msg.mass;
                        Car::mbx[msg.id]->put(msg_t{0,PASS,(dir_t)0,0});
                    }else if( msg.dir != current_dir && current_mass==0){
                        current_dir = msg.dir;
                        current_mass += msg.mass;
                        Car::mbx[msg.id]->put(msg_t{0,PASS,(dir_t)0,0});
                    }else{
                        Car::mbx[msg.id]->put(msg_t{0,WAIT,(dir_t)0,0});
                        wait_list.push_back(msg);
                    }
                    break;
                default:
                    throw ERR_INVALID_OP;
            }
        }
    }
    ~OldBridge(){
        join();
    }
};

MonitorMessageBox<msg_t>* bridge_mbx;
int main(){
    srand(539235);
    
    bridge_mbx = new MonitorMessageBox<msg_t>(10,"oldbridge");

    OldBridge olb(*bridge_mbx);

    ThreadGenerator<Car,MonitorMessageBox<msg_t>&> car_gen(1,3,*bridge_mbx);

    olb.start();
    car_gen.start();

    return 0;
}