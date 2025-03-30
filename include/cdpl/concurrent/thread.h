/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_CONCURRENT_THREAD_H_
#define CDPL_CONCURRENT_THREAD_H_

#include "../utils/log.h"

#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <string>

namespace cdpl {
	inline namespace concurrent {
		class thread {
		public:
			using id = size_t;
			using atomic_id = std::atomic_size_t;

			enum class status {
				idle,
				active,
				finished
			};
			struct descriptor {
				std::string name;
				thread::status status = thread::status::idle;
				thread::id id = ++thread::next_id;
			};
			
			thread(const std::string& name = "thread") : desc_{ name } {}
			virtual ~thread() {}

			thread(const thread&) = delete;
			thread& operator=(const thread&) = delete;

			void start() {
				// TODO: do we need a lock and escape multiple starts, or should terminate if detected?
				std::unique_lock<std::mutex> lock{ mutex_ };
				if (desc_.status == status::idle) {
					thread_ = std::make_unique<std::thread>(thread_runner, this);
					desc_.status = status::active;
					if constexpr (cdpl::log::enabled)
						cdpl::log::debug("thread", "%s[#%u]: started", desc_.name.c_str(), desc_.id);
				}
			}

			void join() {
				if (thread_ && thread_->joinable()) {
					if constexpr (cdpl::log::enabled)
						cdpl::log::debug("thread", "%s[#%u]: joined", desc_.name.c_str(), desc_.id);
					thread_->join();
				}
			}

			descriptor get_descriptor() const {
				std::unique_lock<std::mutex> lock{ mutex_ };
				return desc_;
			}

			// allow the user to change thread's name whenever he wants as it is useful for debugging
			void set_name(const std::string& name) {
				std::unique_lock<std::mutex> lock{ mutex_ };
				desc_.name = name;
			}

			virtual void run() = 0;
		protected:
			// sleep_for alias that shall be used only from inside function so always targets this_thread
			template< class Rep, class Period>
			void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration) {
				std::this_thread::sleep_for(sleep_duration);
			}
		private:
			static void thread_runner(thread* thread) {
				thread->run();
				std::unique_lock<std::mutex> lock{ thread->mutex_ };
				thread->desc_.status = status::finished;
			}
			static atomic_id next_id;

			std::unique_ptr<std::thread> thread_;
			descriptor desc_;
			mutable std::mutex mutex_;
		};

		thread::atomic_id thread::next_id{ 0 };
	}
}



#endif //CDPL_CONCURRENT_THREAD_H_
