/*
	This example is part of Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

	..............................................................................

	Demonstration of some features. Calculating exp of random numbers
*/

#define DEBUG_LINDA

#include "CDPL.h"
#include <cstdlib>
#include <cmath>

using namespace Linda;
using namespace Concurrent;

constexpr int nums_to_calculate = 3;

/**
 * You can use Linda's eval on regular functions that don't take arguments
 * Inability to pass arguments is enforced design choice because of the way
 * function calls are handled in c/c++
 */
int put_random(){
	return (rand() % 10 + 2);
}

struct RandomGenerator: public Thread{
	void run(){
		while(!rdp("numbers_calculated",nums_to_calculate)){
			sleep_for(std::chrono::seconds(rand() % 5));
			eval("random_num", put_random);
		}
	}
	~RandomGenerator(){ join(); }
};


/**
 * You can use Linda's eval on any class that inherits from Active<T> where
 * T is return type. T=void is forbidden because you cannot put void in tuple
 * space but void* is legal and recommended in such cases
 */
struct Exp: Active<double>{
	double element;
	Exp(double element):element(element){}
	double run(){
		int n = 0;
		/**
		 * Calculating exp is fast on modern machines, for simulation purposes add delay
		 * note: Active is not a Thread (although it uses std::thread under the hood)
		 * Reasoning for this is that these are different use cases and special methods
		 * that Thread provides are not suited here and these classes shouldn't share the
		 * same interface
		 */
		std::this_thread::sleep_for(std::chrono::seconds(rand() % 2 + 1));

		/**
		 * If pointer is passed Linda treats passed argument as a wildcard for any of the
		 * type of the underlying variable, and if matching tuple is found, value that
		 * wildcard points to gets updated 
		 */
		if(rdp("numbers_calculated", &n))
			in("numbers_calculated", &n);
		out("numbers_calculated", ++n);

		return std::exp(element);
	}
};
int main(){
	srand(RANDOM_SEED);
	RandomGenerator rg;
	rg.start();

	for(int i=0; i<nums_to_calculate; i++){
		int num;
		in("random_num",&num);
		std::cout << "got new num: " << num << std::endl;
		eval("exp", num, Exp(num));
	}

	rd("numbers_calculated",nums_to_calculate);
	return 0;
}