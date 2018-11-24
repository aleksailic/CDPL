#include "req.h"
#include <cstdlib>

const int demo_buffer_size = 4;

template <class T>
class Buffer: public Monitorable{
	uint capacity, *data;
	uint front = 0, rear = 0, my_size = 0;

	cond space_avail = cond_gen(), item_avail = cond_gen();

	void print(){
		std::cout<<"BUFFER:[ ";
		for(uint iter = front; iter != rear;)
			std::cout << data[iter = (iter + 1) % capacity] << ' ';
		std::cout<<']'<<std::endl;
	}
public:
	Buffer(Mutex& mutex, uint capacity = 10):Monitorable(mutex),capacity(capacity){
		data = new uint[capacity]{0}; //0initialize
	}
	void put(T& elem){
		while(full())
			space_avail.wait();
		my_size++;
		rear = (rear + 1) % capacity;
		data[rear] = elem;
		print();
		item_avail.signal();
	}
	T take(){
		while(empty())
			item_avail.wait();

		my_size--;
		front = (front + 1) % capacity;
		print();
		space_avail.signal();
		return data[front];
	}
	bool empty() const { return front==rear; }
	bool full() const { return rear+1 == front; }
	uint size() const { return my_size; }
};

class Producer: public Thread{
	monitor<Buffer<uint>>& buffer;
	uint sleep_ms, sleep_variance;
public:
	Producer(monitor<Buffer<uint>>& buffer, uint sleep_ms = 1000, uint sleep_variance = 450)
		:buffer(buffer),sleep_ms(sleep_ms),sleep_variance(sleep_variance){}	
	void run() override{
		while(true){
			usleep( (sleep_ms - sleep_variance/2 + (rand() % sleep_variance)) * 1000 );

			uint product = rand() % 20;
			std::cout<<"produced: "<< product << std::endl;
			buffer->put(product);			
		}
	}
	~Producer(){
		join();
	}
};

class Consumer: public Thread{
	monitor<Buffer<uint>>& buffer;
	uint sleep_ms, sleep_variance;
public:
	Consumer(monitor<Buffer<uint>>& buffer, uint sleep_ms = 1000, uint sleep_variance = 450)
		:buffer(buffer),sleep_ms(sleep_ms),sleep_variance(sleep_variance){}	
	void run() override{
		while(true){
			usleep( (sleep_ms - sleep_variance/2 + (rand() % sleep_variance)) * 1000 );

			uint product = buffer->take();
			std::cout<<"consumed: "<< product << std::endl;
		}
	}
	~Consumer(){
		join();
	}
};
using namespace std;
int main(){
	srand(random_seed);
	monitor<Buffer<uint>>*buffer = new monitor<Buffer<uint>>(demo_buffer_size);

	Consumer consumer_thread(*buffer,1000,2000);
	Producer producer_thread(*buffer,1000,2000);

	consumer_thread.start();
	producer_thread.start();
	return 0;
}