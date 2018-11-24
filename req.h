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

class Mutex{
	pthread_mutex_t mutex;
public:
	Mutex(){
		pthread_mutex_init(&mutex,NULL);
	};
	Mutex(pthread_mutex_t mutex):mutex(mutex){}
	void lock(){
		pthread_mutex_lock(&mutex);
	}
	void unlock(){
		pthread_mutex_unlock(&mutex);
	}
	~Mutex(){
		pthread_mutex_destroy(&mutex);
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
		class th_queue{
			struct node{
				uint rank = 0; //default rank
				node* next = nullptr;
				Semaphore mutex={0}; //mutex_t can't be initialized to 0 by standard
			};
			node* head = nullptr;
			uint length = 0;
		public:
			void put_and_lock(pthread_mutex_t* mutex, uint rank=0){
				length++;
				node**iter = &head;
				for(; *iter!=nullptr && (*iter)->rank <= rank; iter = &((*iter)->next));
				*iter = new node{rank,*iter};
				pthread_mutex_unlock(mutex);

				(*iter)->mutex.wait();

				//pthread_mutex_lock(mutex); //upon release try lock the mutex again, no need for additional overhead
			}
			void pop_and_unlock(){
				if(head != nullptr){
					length--;
					node* el = head;
					head = head->next;
					el->mutex.signal();
					delete el;
				}
			}
			void pop_and_unlock_all(){
				length=0;
				node* del_iter = head; //save for deletion
				for(node**iter = &head; *iter!=nullptr; *iter = (*iter)->next)
					(*iter)->mutex.signal();
				while(del_iter){
					node*to_delete = del_iter;
					del_iter = del_iter->next;
					delete to_delete;
				}
			}
			uint minrank() const {return head ? head->rank : -1;} //this will give MAXINT minrank if ever called without check
			uint count() const {return length;}
			bool empty() const {return length==0 ? true : false;}
		};
		Mutex& monitor_mutex;
		pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
		th_queue thq;
		int num_blocked=0;
	public:
		cond(Mutex& monitor_mutex):monitor_mutex(monitor_mutex){}
		void wait(uint priority=0){
			pthread_mutex_lock(&mutex);
			#ifdef DEBUG
				std::cout<< "COND: BLOCK! " << std::endl ;
			#endif
			monitor_mutex.unlock(); //unlock monitor
			thq.put_and_lock(&mutex,priority);
			#ifdef DEBUG
				std::cout<< "COND: RELEASE! " << std::endl ;
			#endif
			//pthread_mutex_unlock(&mutex); if put_and_lock shall lock mutex than we need this
			monitor_mutex.lock(); //try lock the monitor again once it is released
		}
		void signal(){
			pthread_mutex_lock(&mutex);
			if(!thq.empty())
				thq.pop_and_unlock();
			pthread_mutex_unlock(&mutex);
		}
		void signalAll(){
			pthread_mutex_lock(&mutex);
			if(!thq.empty())
				thq.pop_and_unlock_all();
			pthread_mutex_unlock(&mutex);
		}
		bool empty() const{return thq.empty();}
		bool queue() const{return !thq.empty();}
		uint minrank() const {return thq.minrank();}
	};
	class condition_generator{
		Mutex& monitor_mutex;
	public:
		condition_generator(Mutex& monitor_mutex):monitor_mutex(monitor_mutex){}
		cond operator()(){return cond(monitor_mutex);}
	};
	condition_generator cond_gen;
public:
	Monitorable(Mutex& monitor_mutex):cond_gen(monitor_mutex){}
};

template <class T>
class Monitor{
	static_assert(std::is_base_of<Monitorable, T>::value, "T must inherit from Monitorable");
protected:
	Mutex mutex;
	T obj;

	class helper{
		Monitor* mon;
	public:
		helper(Monitor* mon):mon(mon){
			#ifdef DEBUG
				std::cout<<"MONITOR: TRYING TO LOCK"<<std::endl;
			#endif
			mon->mutex.lock();
			#ifdef DEBUG
				std::cout<<"MONITOR: LOCKING"<<std::endl;
			#endif
		}
		~helper(){
			mon->mutex.unlock();
			#ifdef DEBUG
				std::cout<<"MONITOR: UNLOCKING"<<std::endl;
			#endif
		}
		T* operator->(){return &mon->obj;}
	};

public:
	template <typename ...Args>
	Monitor(Args&&... args):obj(mutex,std::forward<Args>(args)...){
		#ifdef DEBUG
		std::cout<<"MONITOR: CREATED"<<std::endl;
		#endif
	}
	helper operator->(){return helper(this);}
	T& operator*(){return obj;} //return object without monitor
};

/*
template <typename T>
class Shared{
	T obj;
	//std::vector<cond> conditions;
	//std::vector<Semaphore> events;
	Semaphore mutex;
public:
	template <typename ...Args>
	Shared(Args&&... args):mutex(1),obj(std::forward<Args>(args)...){
		#ifdef DEBUG
		std::cout<<"SHARED: CREATED"<<std::endl;
		#endif
	}
	Shared& operator=(T&& rhs){
		obj = std::move(rhs);
		return *this;
	}
	void await(cond&& condition){
		//problem: how to make sure events is not already registered? 
		//conditions.push_back(std::forward<cond>(condition));
		//events.push_back(Semaphore(condition() ? 1 : 0)); //run condition at once to see whether to block on its semaphore
	}
}
*/

//Some aliases for easier usage
typedef Semaphore sem;
template<typename T> using monitor = Monitor<T>;
//template<typename T> using shared = Shared<T>;

#endif