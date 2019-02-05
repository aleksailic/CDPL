/*
	This example is part of Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

	..............................................................................

	Producer-consumer problem: 2 processes coexist, the producer and the consumer,
	who share a common, fixed-size buffer used as a queue. The producer's job is
	to generate data, put it into the buffer, and start again. At the same time,
	the consumer is consuming the data (i.e., removing it from the buffer),
	one piece at a time. Make sure that the producer won't try to add data into
	the buffer if it's full and that the consumer won't try to remove data from an
	empty buffer.

	Solution using monitor
*/

#include "CDPL.h"
#include <cstdlib>
#include <unistd.h>
using namespace Concurrent;
using namespace Testbed;
constexpr int demo_buffer_size = 4;

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
			std::this_thread::sleep_for(std::chrono::milliseconds((
				sleep_ms - sleep_variance/2 + (rand() % sleep_variance)) * 1000
			));

			uint product = rand() % 20;
			std::cout<<"-- produced: "<< product << std::endl;
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
			std::this_thread::sleep_for(std::chrono::milliseconds((
				sleep_ms - sleep_variance/2 + (rand() % sleep_variance)) * 1000
			));
			uint product = buffer->take();
			std::cout<<"-- consumed: "<< product << std::endl;
		}
	}
	~Consumer(){
		join();
	}
};

static monitor<Buffer<uint>> buffer(demo_buffer_size);
using namespace std;
int main(){
	srand(random_seed);
	Consumer consumer_thread(buffer,1000,2000);
	Producer producer_thread(buffer,1000,2000);

	consumer_thread.start();
	producer_thread.start();
	return 0;
}
