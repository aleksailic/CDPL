/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_CONCURRENT_MONITOR_H_
#define CDPL_CONCURRENT_MONITOR_H_

#include "atomic.h"
#include "condition_variable.h"

namespace cdpl {
	inline namespace concurrent {
		class monitorable {
			template <class T>
			friend class monitor;
		protected: 
			class condition_binder_t {
				template <class T>
				friend class monitor;
			public:
				cdpl::condition_variable operator()() {
					return cdpl::condition_variable(*mutex_);
				}
			protected:
				void set_mutex(std::mutex* mutex) { mutex_ = mutex; }
				std::mutex* mutex() { return mutex_; }
			private:
				std::mutex* mutex_ = nullptr;
			};
			condition_binder_t condition_binder;
		};

		template <class T>
		class monitor : private cdpl::atomic<T> {
			static_assert(std::is_base_of<cdpl::monitorable, T>::value, "T must inherit from Monitorable");
		public:
			template <typename ...Args>
			monitor(Args&&... args) : cdpl::atomic<T>(T{ args... }) {
				object_.condition_binder.set_mutex(&mutex_);
				if constexpr (cdpl::log ::enabled) cdpl::log::debug("monitor", "created");
			}

			~monitor() {
				if constexpr (cdpl::log::enabled) cdpl::log::debug("monitor", "destroyed");
			}

			using cdpl::atomic<T>::operator->;
			using cdpl::atomic<T>::operator*;
		};
	}
}

#endif // CDPL_CONCURRENT_MONITOR_H_
