/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "test.hpp"

int main(int argc, char* argv[]) {
	int result = Catch::Session().run(argc, argv);

	system("pause");
	return result;
}