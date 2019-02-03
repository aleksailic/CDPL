/*
    This file is part of Concurrent and Distributed Programming Library for C++
    Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#define DEBUG_LINDA
#include "CDPL.h"
#include <unistd.h>
using namespace Linda;
using namespace Concurrent;

class A:public Thread{
    void run(){
        in("aleksa",2);
        std::cout<<"ALL GOOD NOW!";
    }
};
int main(){
    out("aleksa",1);

    in("aleksa",1);
    A a;
    a.start();
    
    sleep(4);
    out("aleksa",2);

    a.join();
    
    return 0;
}