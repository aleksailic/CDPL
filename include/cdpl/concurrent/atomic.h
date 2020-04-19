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
	inline namespace concurrent {
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
			atomic() = delete;

			explicit atomic(const atomic<T>& rhs) : object_(rhs.object_) {}
			explicit atomic(atomic<T>&& rhs) noexcept : object_(std::move(rhs.object_)) {}

			atomic(const T& rhs) : object_(rhs) {}
			atomic(T&& rhs) noexcept : object_(std::move(rhs)) {}

			atomic& operator=(const T& object){
				std::unique_lock<mutex> lock(mutex_);
				cv_.notify_all();
				object_ = object;
				return *this;
			}

			friend bool operator==(const atomic& lhs, const T& rhs){
				std::unique_lock<cdpl::mutex> lock(lhs.mutex_);
				return lhs.object_ == rhs;
			}
			friend bool operator==(const T& lhs, const atomic& rhs){
				return rhs == lhs;
			}
			friend bool operator!=(const atomic& lhs, const T& rhs){ 
				return !(lhs == rhs);
			}
			friend bool operator!=(const T& lhs, const atomic& rhs){
				return !(rhs == lhs);
			}
			friend bool operator<(const atomic& lhs, const T& rhs){
				std::unique_lock<cdpl::mutex> lock(lhs.mutex_);
				return lhs.object_ < rhs;
			}
			friend bool operator>(const atomic& lhs, const T& rhs){
				std::unique_lock<cdpl::mutex> lock(lhs.mutex_);
				return lhs.object_ > rhs;
			}

			// Call underlying object's function in mutually exclusive way.
			atomic_helper operator->() {
				return atomic_helper(this);
			}
			T operator*() {
				return get_copy();
			}
			operator T() const noexcept {
				return get_copy();
			}

			// Returns a reference to the managed object
			T& get() {
				return object_;
			}
			T get_copy() const {
				std::unique_lock<cdpl::mutex> lock(mutex_);
				return object_;
			}
			// Lockable
			bool try_lock() {
				return mutex_.try_lock();
			}
			void lock() {
				mutex_.lock();
			}
			void unlock() {
				mutex_.unlock();
			}
			// Wait on change
			void wait() {
				std::unique_lock<cdpl::mutex> lock(mutex_);
				cv_.wait(lock);
			}
			void notify_one() noexcept{
				cv_.notify_one();
			}
			void notify_all() noexcept{
				cv_.notify_all();
			}
		private:
			mutable cdpl::mutex mutex_; //atomic's mutex
			T object_; //hidden atomic object which methods will be called
			mutable std::condition_variable cv_;
		};
	}
}



#endif //CDPL_CONCURRENT_ATOMIC_H_