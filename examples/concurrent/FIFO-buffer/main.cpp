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

#define DEBUG_BUFFER

#include "CDPL.h"
#include <cstdlib>

using namespace Concurrent;
using namespace Testbed;
using namespace std::chrono_literals;

constexpr uint  buffer_size = 4;
constexpr auto sleep_duration = 2s;
constexpr auto sleep_variance = 1500ms;
constexpr uint  num_of_consumers = 2;
constexpr uint  num_of_producers = 3;

template <class T>
class Buffer: public Monitorable{
	T* data;
	uint front = 0, rear = 0, my_size = 0, capacity;

	cond space_avail = cond_gen("space_avail"), item_avail = cond_gen("item_avail");

	void print(){
		std::cout<<"BUFFER: [";
		for(uint iter = front; iter != rear;)
			std::cout << data[iter = (iter + 1) % capacity] << ' ';
		std::cout<<']'<<std::endl;
#ifdef DEBUG_BUFFER
		fprintf(DEBUG_STREAM, "-- buffer vars: front(%d) rear(%d) my_size(%d)\n", front, rear, my_size);
#endif
	}
public:
	Buffer(uint capacity = 10):capacity(capacity + 1){
		data = new T[capacity + 1];
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
	bool full() const { return (rear + 1) % capacity == front; }
	uint size() const { return my_size; }
};

typedef monitor<Buffer<uint>> buffer_m;

class Producer: public Thread{
	buffer_m& buffer;
public:
	Producer(buffer_m& buffer):Thread("producer"), buffer(buffer){}
	void run() override{
		while(true){
			sleep_for(sleep_duration - sleep_variance/2  + ((float)rand()/(float)(RAND_MAX))*sleep_variance);
			uint product = rand() % 20;
#ifdef DEBUG_BUFFER
			fprintf(DEBUG_STREAM, "-- produced: %d\n", product);
#endif
			buffer->put(product);
		}
	}
	~Producer(){ join(); }
};

class Consumer: public Thread{
	buffer_m& buffer;
public:
	Consumer(buffer_m& buffer):Thread("consumer"), buffer(buffer){}
	void run() override{
		while(true){
			sleep_for(sleep_duration - sleep_variance/2  + ((float)rand()/(float)(RAND_MAX))*sleep_variance);
			uint product = buffer->take();
#ifdef DEBUG_BUFFER
			fprintf(DEBUG_STREAM, "-- consumed: %d\n", product);
#endif
		}
	}
	~Consumer(){ join(); }
};

static buffer_m buffer(buffer_size);
int main(){
	srand(random_seed);

	std::vector<Consumer> consumers {num_of_consumers, buffer};
	std::vector<Producer> producers {num_of_producers, buffer};
	
	for(auto& consumer: consumers)
		consumer.start();
	for(auto& producer: producers)
		producer.start();
	return 0;
}
