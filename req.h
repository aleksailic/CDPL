#ifndef REQ_H
#define REQ_H

#include <pthread.h>
#include <semaphore.h>
#include <iostream> 

#define DEBUG

typedef unsigned int uint;
class Semaphore{
	int val;
	sem_t sid;
public:
	Semaphore(uint val=0):val(val){
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
	int debug;
	Semaphore mysem;
public:
	Thread(const char * name = "thread"):name(name),debug(debug),mysem(0){}
	void start(){
		pthread_create(&pid,NULL,Thread::fn_caller,this);
		#ifdef DEBUG
			std::cout<< "instance of " << name << " started" << std::endl;
		#endif
	}
	void pause(){
		mysem.wait();
	}
	void resume(){
		mysem.signal();
	}
	void join(){
		#ifdef DEBUG
			std::cout<< "waiting for " << name << " to join" << std::endl ;
		#endif
		pthread_join(pid,NULL);
	}
};

template <class T>
class Monitor{
protected:
	T obj;
	Semaphore mutex;

	class helper{
		Monitor* mon;
	public:
		helper(Monitor* mon):mon(mon){
			#ifdef DEBUG
				std::cout<<"TRYING TO LOCK"<<std::endl;
			#endif
			mon->mutex.wait();
			#ifdef DEBUG
				std::cout<<"LOCKING"<<std::endl;
			#endif
		}
		~helper(){
			mon->mutex.signal();
			#ifdef DEBUG
				std::cout<<"UNLOCKING"<<std::endl;
			#endif
		}
		T* operator->(){return &mon->obj;}
	};

public:
	template <typename ...Args>
	Monitor(Args&&... args):obj(std::forward<Args>(args)...),mutex(1){
		#ifdef DEBUG
		std::cout<<"MONITOR CREATED"<<std::endl;
		#endif
	}
	helper operator->(){return helper(this);}
};

//Some aliases for easier usage
typedef Semaphore sem;
template<typename T> using monitor = Monitor<T>;

#endif