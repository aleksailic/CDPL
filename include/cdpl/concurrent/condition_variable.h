/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_CONCURRENT_CONDITION_VARIABLE_H
#define CDPL_CONCURRENT_CONDITION_VARIABLE_H

#include <mutex>
#include <atomic>
#include <queue>

namespace cdpl {
	inline namespace concurrent {
		template <class Mutex> 
		class basic_condition_variable {
		public:
			basic_condition_variable(Mutex& mutex) : mutex_(std::ref(mutex)) {}

			// Blocks the current process until the condition variable is woken up.
			void wait() {
				blocked_num++;
				std::unique_lock<std::mutex> lock(shared_mutex_);
				cv_.wait(lock);
			}
			
			// Unblock one process from blocked queue
			void notify_one() {
				blocked_num--;
				cv_.notify_one();
			}
			
			// Unblocks all processes from blocked queue
			void notify_all() {
				blocked_num = 0;
				cv_.notify_all();
			}

			// Check whether blocked queue is empty
			bool empty() {
				return blocked_num == 0;
			}

			// Check if there are processes in blocked queue
			bool queue() {
				return !empty();
			}
		private:
			mutable std::condition_variable cv_;
			mutable std::reference_wrapper<Mutex> mutex_;
			std::atomic_uint blocked_num = 0;
		};
	
		/*template <class Mutex>
		class basic_priority_condition_variable {
		public:
			basic_priority_condition_variable(std::shared_ptr<Mutex> mutex) : shared_mutex_(std::move(mutex)) {}
		
			void wait(unsigned int priority = 0) {
				std::unique_lock<std::mutex> lock(mutex_);
				thq.emplace(priority);
			}
		private:
			struct node {
				unsigned int rank;
				cdpl::sem sem;

				node(unsigned int rank) : rank(rank) {}
			};

			std::priority_queue<node, std::vector<node>, std::greater<node>> thq;
			mutable std::shared_ptr<Mutex> shared_mutex_;
			mutable std::mutex mutex_;
		};
		*/

		using condition_variable = basic_condition_variable<std::mutex>;
	}
}

#endif //CDPL_CONCURRENT_CONDITION_VARIABLE_H
