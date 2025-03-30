/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_CONCURRENT_MUTEX_H_
#define CDPL_CONCURRENT_MUTEX_H_

#include <mutex>
#include <shared_mutex>

namespace cdpl {
	inline namespace concurrent {
		using mutex = std::mutex;
		using shared_mutex = std::shared_mutex;
	}
}

#endif //CDPL_CONCURRENT_MUTEX_H_
