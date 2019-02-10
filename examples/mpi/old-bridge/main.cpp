/*
    This example is part of Concurrent and Distributed Programming Library for C++
    Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    ..............................................................................

    Old bridge problem: Citizens of Brzi Brod have an old bridge that has only
    one lane. Cars are coming from NORTH and SOUTH. Each car has its own mass.
    Old bridge can take MAX_MASS before it "izrazito" collapses. Help the citizens
    of Brzi Brod pass the bridge.

    Solution similar to one-lane-bridge example given in concurrent folder, just
    needs to utilize MPI as means of communication and no optimization for direction
    flip on max car pass.
*/

#include "CDPL.h"
#include <cstdlib>

#define N_CARS 20
#define MAX_MASS 201

using namespace Concurrent;
using namespace MPI;
using namespace Testbed;
using namespace Utils;

enum dir_t {NORTH,SOUTH};
enum op_t  {WAIT,PASS,ENTER,EXIT};
enum err_t {ERR_INVALID_OP,ERR_CAR_OVERFLOW};

struct msg_t{
    uint  id;
    op_t  op;
    dir_t dir;
    uint  mass;
};
typedef MonitorMessageBox<msg_t> MailBox;

class Car: public Thread{
    MailBox& bridge_mbx;
    std::string name;
    dir_t direction;
    uint  id;
    uint  mass;
public:
    static MailBox mbx[N_CARS];
    static std::atomic<uint> next_id;
    Car(MailBox& bridge_mbx, dir_t direction, uint mass): bridge_mbx(bridge_mbx), id(next_id++), direction(direction), mass(mass){
        if(id == N_CARS) throw ERR_CAR_OVERFLOW;
        name = string_format("CAR[%c#%d]", direction == SOUTH ? 'S' : 'N', id);
        Thread::set_name(name.data());
        std::cout << lock << name << colorize(" created", TC::YELLOW) << std::endl << unlock;
    }
    Car(MailBox& bridge_mbx, dir_t direction): Car(bridge_mbx, direction, rand() % 70 + 30) { }
    void run() override{
        bridge_mbx->put(msg_t{id,ENTER,direction,mass});
        msg_t msg{0,WAIT};
        while(msg.op == WAIT)
            msg = mbx[id]->get();

        std::cout << lock << name << " is " << colorize("passing",TC::RED, TS::BOLD) << std::endl << unlock;
        sleep_for(std::chrono::seconds(rand() % 4 + 3));

        bridge_mbx->put(msg_t{id,EXIT,direction,mass});
        std::cout << lock << name << colorize(" exiting", TC::GREEN) << std::endl << unlock;
    }
};
MailBox Car::mbx[N_CARS];
std::atomic<uint> Car::next_id {0};

class OldBridge: public Thread{
    MailBox& mbx;
    dir_t current_dir = SOUTH;
    uint current_mass = 0;
    std::list<msg_t> wait_list;
public:
    OldBridge(MailBox&mbx): Thread("OldBridge"), mbx(mbx){}
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

static MailBox bridge_mbx {10, "oldbridge"};

int main(){
    srand(539235);

    OldBridge olb(bridge_mbx);
    olb.start();

	std::vector<ThreadGenerator<Car>> generators = { {1s, 5s, bridge_mbx, NORTH}, {1s, 5s, bridge_mbx, SOUTH} };
	for(auto& generator: generators)
		generator.start();

    return 0;
}
