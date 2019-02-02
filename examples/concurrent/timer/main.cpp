#include "kdp.h"

#include <ctime>
#include <cstdlib>

using namespace Concurrent;
using namespace Testbed;

class Alarm:public Monitorable{
	uint now = time(NULL); //current time
	cond wakeup = cond_gen();
public:
	class TickThread:public Thread{
		Alarm& my_alarm;
		void run(){
			my_alarm.now=time(NULL);
			std::cout<<"TIME: "<< my_alarm.now<<std::endl;
			if(!my_alarm.wakeup.empty() && my_alarm.wakeup.minrank()<=my_alarm.now)
				my_alarm.wakeup.signal();
		}
	public:
		TickThread(Alarm& alarm):my_alarm(alarm){}
	};
	Alarm(Mutex& mutex):Monitorable(mutex){}

	void wake_me(uint ticks_from_now){
		wakeup.wait(ticks_from_now);
		if(!wakeup.empty() && wakeup.minrank()<=now)
			wakeup.signal();
	}
};

struct Worker:public Thread{
	monitor<Alarm>& my_alarm;
	Worker(monitor<Alarm>& my_alarm):my_alarm(my_alarm){}
	void run() override{
		uint wake_from  = rand() % 10;
		std::cout << "I WAKE " << wake_from << "secs from now!" << std::endl;
		my_alarm->wake_me(time(NULL)+wake_from);
		std::cout << "AWAKE!"<<std::endl;
	}
};

using namespace std;
int main(){
	srand(random_seed);
	monitor<Alarm>*alarm = new monitor<Alarm>;
	ThreadGenerator<Alarm::TickThread,Alarm&> alarm_tick_generator(1,1,**alarm); //generate tick every second
	ThreadGenerator<Worker,monitor<Alarm>&> worker_generator(4,8,*alarm);
	alarm_tick_generator.start();
	worker_generator.start();
	return 0;
}