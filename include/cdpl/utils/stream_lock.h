/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_UTILS_STREAM_LOCK_H_
#define CDPL_UTILS_STREAM_LOCK_H_

#include <iostream>
#include <mutex>

namespace cdpl {
	namespace utils {
		enum class stream_lock_directive { lock, unlock };

		constexpr auto lock = stream_lock_directive::lock;
		constexpr auto unlock = stream_lock_directive::unlock;

		// use with caution as it can lead to deadlock easily
		std::ostream& operator<<(std::ostream& os, const stream_lock_directive directive) {
			static std::mutex stream_mutex;
			if (directive == stream_lock_directive::lock)
				stream_mutex.lock();
			else if (directive == stream_lock_directive::unlock)
				stream_mutex.unlock();

			return os;
		}
	}
}

#endif //CDPL_UTILS_STREAM_LOCK_H_