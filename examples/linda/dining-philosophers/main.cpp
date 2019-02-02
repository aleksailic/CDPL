#define DEBUG_LINDA
#define DEUBG_PHILOSOPHERS
#include "kdp.h"
#include <cstdlib>
using namespace Linda;

#define N 5

class Philosopher: public Active<void*>{
	int id;
	inline void think(){
		#ifdef DEUBG_PHILOSOPHERS
			fprintf(DEBUG_STREAM, "Philosopher(%d) is thinking\n",id);
		#endif
		std::this_thread::sleep_for(std::chrono::seconds(rand()%4));
		#ifdef DEUBG_PHILOSOPHERS
			fprintf(DEBUG_STREAM, "Philosopher(%d) finished thinking\n",id);
		#endif
	}
	inline void eat(){
		#ifdef DEUBG_PHILOSOPHERS
			fprintf(DEBUG_STREAM, "Philosopher(%d) is eating\n",id);
		#endif
		std::this_thread::sleep_for(std::chrono::seconds(rand()%4));
		#ifdef DEUBG_PHILOSOPHERS
			fprintf(DEBUG_STREAM, "Philosopher(%d) finished eating\n",id);
		#endif
	}
public:	
	Philosopher(int id):id(id){}
	void* run() override{
		int left=id, right=(id+1)%N;
		while(1){
			think();
			in("ticket");
			in("fork",left);
			in("fork",right);
			eat();
			out("fork",left);
			out("fork",right);
			out("ticket");
		}
		return nullptr;
	}
};

int main(){
	srand(RANDOM_SEED);
	#ifdef DEUBG_PHILOSOPHERS
		fprintf(DEBUG_STREAM, "-- init: %d philosophers --\n",N);
	#endif
	for(int i=0;i<N;i++){
		out("fork",i);
		eval(Philosopher(i));
		if(i<N-1)out("ticket");
	}
	in("finish");
}