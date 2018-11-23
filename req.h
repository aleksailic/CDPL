#ifndef REQ_H
#define REQ_H

#include <pthread.h>
#include <semaphore.h>
#include <iostream> 
#include <type_traits>
#include <signal.h>
#include <random>
#include <unistd.h>
#include <tuple>

typedef unsigned int uint;
class Semaphore{
	sem_t sid;
public:
	Semaphore(uint val=0){
		sem_init(&sid,0,val);
	};
	~Semaphore(){
		sem_destroy(&sid);
	}
	void wait(){
		sem_wait(&sid);
	}
	void signal(){
		sem_post(&sid);
	}
};

class Thread{
	static void* fn_caller(void * arg){
		static_cast<Thread*>(arg)->run();
	}
	virtual void run()=0;
	pthread_t pid;
	const char * name;
	Semaphore mysem;
public:
	Thread(const char * name = "thread"):name(name),mysem(0),pid(0){}
	void start(){
		pthread_create(&pid,NULL,Thread::fn_caller,this);
		#ifdef DEBUG
			std::cout<< "THREAD["<<pid<<"]: "<< name << " started" << std::endl;
		#endif
	}
	void pause(){
		mysem.wait();
	}
	void resume(){
		mysem.signal();
	}
	void kill(){
		if(pid) pthread_kill(pid,0);
	}
	void join(){
		#ifdef DEBUG
			std::cout<< "waiting for " << name << " to join" << std::endl ;
		#endif
		if(pid) pthread_join(pid,NULL);
	}
};

const int random_seed = 836939;
template <class T,typename ...Args>
class ThreadGenerator:public Thread{
	static_assert(std::is_base_of<Thread, T>::value, "T must inherit from Thread");
	std::default_random_engine random_engine;
	std::uniform_int_distribution<uint> random_generator;
	std::tuple<Args&&...> stored_args;

	//helper function for generating threads from stored arguments, really hacky stuff...
	template<std::size_t... Is>
	T* generateThread(const std::tuple<Args&&...>& tuple,
    	std::index_sequence<Is...>){
		return new T(std::get<Is>(tuple)...);
	}

	void run(){
		while(true){
			sleep(random_generator(random_engine));
			generateThread(stored_args,std::index_sequence_for<Args&&...>())->start();
		}
	}
public:
	ThreadGenerator(uint min_seconds=1, uint max_seconds=3,Args&&... args)
		:random_engine(random_seed), random_generator(min_seconds,max_seconds),stored_args(std::forward<Args>(args)...){}
	~ThreadGenerator(){
		join();
	}
};


class Monitorable{
protected:
	class cond{
		Semaphore& monitor_mutex;
		Semaphore sem;
		Semaphore mutex;
		int num_blocked=0;
	public:
		cond(Semaphore& monitor_mutex):monitor_mutex(monitor_mutex),mutex(1){}
		void wait(){
			mutex.wait();
			num_blocked++;
			#ifdef DEBUG
				std::cout<< "COND: BLOCK! " << std::endl ;
			#endif
			monitor_mutex.signal(); //unlock monitor
			mutex.signal();
			sem.wait();
			#ifdef DEBUG
				std::cout<< "COND: RELEASE! " << std::endl ;
			#endif
			monitor_mutex.wait(); //try lock the monitor again once it is released
		}
		void signal(){
			mutex.wait();
			if(num_blocked){
				num_blocked--;
				sem.signal();
			}
			mutex.signal();
		}
		void signalAll(){
			mutex.wait();
			while(num_blocked){
				num_blocked--;
				sem.signal();
			}
			mutex.signal();
		}
		bool empty() const{return !((bool)num_blocked);}
	};
	class condition_generator{
		Semaphore& monitor_mutex;
	public:
		condition_generator(Semaphore& monitor_mutex):monitor_mutex(monitor_mutex){}
		cond operator()(){return cond(monitor_mutex);}
	};
	condition_generator cond_gen;
public:
	Monitorable(Semaphore& monitor_mutex):cond_gen(monitor_mutex){}
	enum DISCIPLINE{SIGNAL_AND_WAIT,SIGNAL_AND_CONTINUE};
};

template <class T>
class Monitor{
	static_assert(std::is_base_of<Monitorable, T>::value, "T must inherit from Monitorable");
protected:
	Semaphore mutex;
	T obj;

	class helper{
		Monitor* mon;
	public:
		helper(Monitor* mon):mon(mon){
			#ifdef DEBUG
				std::cout<<"MONITOR: TRYING TO LOCK"<<std::endl;
			#endif
			mon->mutex.wait();
			#ifdef DEBUG
				std::cout<<"MONITOR: LOCKING"<<std::endl;
			#endif
		}
		~helper(){
			mon->mutex.signal();
			#ifdef DEBUG
				std::cout<<"MONITOR: UNLOCKING"<<std::endl;
			#endif
		}
		T* operator->(){return &mon->obj;}
	};

public:
	template <typename ...Args>
	Monitor(Args&&... args):mutex(1),obj(mutex,std::forward<Args>(args)...){
		#ifdef DEBUG
		std::cout<<"MONITOR: CREATED"<<std::endl;
		#endif
	}
	T& demonitorize(){return obj;} //get object without monitor properties
	helper operator->(){return helper(this);}
};

//Some aliases for easier usage
typedef Semaphore sem;
template<typename T> using monitor = Monitor<T>;

#endif