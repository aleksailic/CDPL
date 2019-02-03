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