#include "req.h"
#include <iostream>
#include <unistd.h>
#include <vector>

class myClass: public Monitorable{
	cond my_cond = cond_gen();
	int x = 0;
public:
	myClass(Mutex& monitor_mutex):Monitorable(monitor_mutex){}
	void test(){
		x++;
		std::cout<<"x je sada "<<x<<std::endl;
		if(x==1){
			my_cond.wait();
			std::cout<<"x oslobodjeni je "<<x<<std::endl;
		}else if(x==3){
			my_cond.signal();
		}
	}
};

class TestThread:public Thread{
	monitor<myClass>& mon;
	void run() override{
		mon->test();
	}
public:
	TestThread(monitor<myClass>& mon,const char* name="test"):Thread(name),mon(mon){}
	~TestThread(){
		join();
	}
};

using namespace std;
int main(){
	constexpr int num_of_threads = 5;
	monitor<myClass>* mon = new monitor<myClass>;
	std::vector< TestThread > threads (num_of_threads,TestThread(*mon));

	for(auto& thread:threads)
		thread.start();

	return 0;	
}