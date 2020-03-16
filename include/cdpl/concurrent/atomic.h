/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_CONCURRENT_ATOMIC_H_
#define CDPL_CONCURRENT_ATOMIC_H_

#include "../utils/common.h"
#include "mutex.h"

namespace cdpl {
	namespace concurrent {
		template <typename T>
		class atomic {
			/**
			 * Helper class that effectively locks and unlocks the mutex on function call.
			 * This is achieved in RAII fashion, as temporary helper object gets constructed
			 * (thus acquiring mutex) when overloaded operator-> gets called, and immediately
			 * after function call gets destroyed (thus releasing mutex).
			*/
			class atomic_helper {
			public:
				atomic_helper(atomic* instance) : instance_(instance) {
#ifdef DEBUG_ATOMIC
					DEBUG_WRITE("atomic", "trying to lock");
#endif
					instance_->mutex_.lock();
#ifdef DEBUG_ATOMIC
					DEBUG_WRITE("atomic", "locking");
#endif
				}

				~atomic_helper() {
					instance_->mutex_.unlock();
#ifdef DEBUG_ATOMIC
					DEBUG_WRITE("atomic", "unlocked");
#endif
				}

				// Nifty hack to insert underlying object and create illusion of direct access
				T* operator->() {
					return &instance_->object_;
				}
			private:
				atomic* instance_;
			};
		public:
			template <typename ...Args>
			atomic(Args&&... args) : object_(std::forward<Args>(args)...) {}

			// Call underlying object's function in mutually exclusive way.
			atomic_helper operator->() {
				return atomic_helper(this);
			}

			// Returns a reference to the managed object without locking
			T& get() {
				return object_;
			}

			// Returns a copy of the managed object thread safe
			T get_copy() {
				mutex_.lock();
				T copy { get() };
				mutex_.unlock();
				return copy;
			}
		private:
			mutex mutex_; //atomic's mutex
			T object_; //hidden atomic object which methods will be called
		};
	}
}



#endif //CDPL_CONCURRENT_ATOMIC_H_