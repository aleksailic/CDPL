#define DEBUG_LINDA
#define DEBUG_SMOKERS

#include "kdp.h"
#include <cstdlib>
using namespace Linda;

void* agent(){
	while(1){
		int n = rand() % 3;
		switch(n){
			case 0:
				out("Paper");
				out("Tobacco");
				break;
			case 1:
				out("Tobacco");
				out("Matches");
				break;
			case 2:
				out("Matches");
				out("Paper");
				break;
			default:
				throw;
		}
		out("Watch");
		in("OK");
	}
}

struct Smoker: public Active<void*>{
	const char* const has;
	const char* const missing[2];
	inline void enjoy(){
		#ifdef DEBUG_SMOKERS
			fprintf(DEBUG_STREAM,"Smoker(%s) is enjoying\n",has);
		#endif
		std::this_thread::sleep_for(std::chrono::seconds(rand()%6));
		#ifdef DEBUG_SMOKERS
			fprintf(DEBUG_STREAM,"Smoker(%s) finished enjoying\n",has);
		#endif
	}
	Smoker(const char* has, const char* missing1, const char* missing2):has(has),missing{missing1,missing2}{}
	void* run(){
		int n=0;
		while(1){
			in("Watch");
			if(rdp(missing[0]) && rdp(missing[1])){
				in(missing[0]);
				in(missing[1]);
				enjoy();
				in("num_blocked",&n);
				for(int i=0;i<n;i++)
					out("next");
				for(int i=0;i<n;i++)
					in("nextOK");
				out("num_blocked", 0);
				out("OK");
			}else{
				in("num_blocked", &n);
				out("num_blocked", n+1);
				out("Watch");
				in("next");
				out("nextOK");
			}
		}
	}
};

int main(){
	srand(RANDOM_SEED);
	out("num_blocked", 0);
	eval(agent);
	eval(Smoker("Matches","Tobacco","Paper"));
	eval(Smoker("Tobacco","Matches","Paper"));
	eval(Smoker("Paper","Matches","Tobacco"));

	in("finish"); //hang this thread
	return 0;
}