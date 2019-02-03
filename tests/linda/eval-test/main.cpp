/*
	This file is part of Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#define DEBUG_LINDA

#include "CDPL.h"
using namespace Linda;
using namespace Concurrent;

int fn() {
	return 3;
}

class A : public Active<const char *> {
	const char* name = "TEST";
public:
	const char * run() final {
		for (int i = 1; i <= 5; i++) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << i << std::endl;
		}
		return "ALL DONE";
	}
};

int main() {
	srand(432423);


	eval(5,fn);
	eval(5, A());

	in(5, 3);
	in(5, "ALL DONE");

	return 0;
}
