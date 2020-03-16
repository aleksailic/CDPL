/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_CONCURRENT_SEMAPHORE_H_
#define CDPL_CONCURRENT_SEMAPHORE_H_

#include <mutex>
#include <condition_variable>

namespace cdpl {
	namespace concurrent {
		class semaphore {
		public:
			explicit semaphore(int val = 0) : val_(val) {}

			/**
			 * Increments the internal value of semaphore by 1.
			 * If the pre-increment value was negative (there are processes waiting for resource),
			 * it transfers a blocked process from the semaphore's waiting queue to the ready queue
			 */
			void signal() {
				std::unique_lock<std::mutex> lock{ mutex_ };
				if (val_++ < 0)
					cond_.notify_one();
			}

			/**
			 * Decrements the internal value of semaphore by 1. After the decrement,
			 * if the post-decrement value was negative, the process executing wait is blocked
			 * (added to the semaphore's queue). Otherwise, the process continues execution.
			 */
			void wait() {
				std::unique_lock<std::mutex> lock(mutex_);
				if (--val_ < 0)
					cond_.wait(lock);
			}
		private:
			std::mutex mutex_;
			std::condition_variable cond_;
			int val_;
		};
	}
}

#endif //CDPL_CONCURRENT_SEMAPHORE_H_
