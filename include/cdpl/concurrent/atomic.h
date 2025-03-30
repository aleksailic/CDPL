/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_CONCURRENT_ATOMIC_H_
#define CDPL_CONCURRENT_ATOMIC_H_

#include "../utils/log.h"
#include "mutex.h"

namespace cdpl {
	inline namespace concurrent {
		template <class T>
		class monitor;

		template <typename T>
		class atomic {
			friend cdpl::monitor<T>;
			/**
			 * Helper class that effectively locks and unlocks the mutex on function call.
			 * This is achieved in RAII fashion, as temporary helper object gets constructed
			 * (thus acquiring mutex) when overloaded operator-> gets called, and immediately
			 * after function call gets destroyed (thus releasing mutex).
			*/
			class atomic_helper {
			public:
				atomic_helper(atomic* instance) : instance_(instance) {
					instance_->lock();
				}

				~atomic_helper() {
					instance_->unlock();
				}

				// Nifty hack to insert underlying object and create illusion of direct access
				T* operator->() {
					return &instance_->get();
				}
			private:
				atomic* instance_;
			};
		public:
			atomic() = delete;

			// TODO: copy/move constructor only treat underlying object without locking, discuss
			explicit atomic(const atomic<T>& rhs) : object_(rhs.object_) {}
			explicit atomic(atomic<T>&& rhs) noexcept : object_(std::move(rhs.object_)) {}

			atomic(const T& rhs) : object_(rhs) {}
			atomic(T&& rhs) noexcept : object_(std::move(rhs)) {}

			atomic& operator=(const T& object){
				std::unique_lock<mutex> lock(mutex_);
				object_ = object;
				notify_all();
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

			// Returns a reference to the managed object without locking, careful!
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
				if constexpr (cdpl::log::enabled) cdpl::log::debug("atomic", "trying to lock");
				mutex_.lock();
				if constexpr (cdpl::log::enabled) cdpl::log::debug("atomic", "locked");
			}
			void unlock() {
				mutex_.unlock();
				if constexpr (cdpl::log::enabled) cdpl::log::debug("atomic", "unlocked");
			}
			// Wait on change
			void wait() {
				std::unique_lock<cdpl::mutex> lock(cv_mutex_);
				if constexpr (cdpl::log::enabled) cdpl::log::debug("atomic:cv", "waiting on condition");
				cv_.wait(lock);
			}
			void notify_one() noexcept{
				if constexpr (cdpl::log::enabled) cdpl::log::debug("atomic:cv", "notified one thread");
				cv_.notify_one();
			}
			void notify_all() noexcept{
				if constexpr (cdpl::log::enabled) cdpl::log::debug("atomic:cv", "notified all threads");
				cv_.notify_all();
			}
		private:
			mutable cdpl::mutex cv_mutex_; // mutex for condition variable
			mutable cdpl::mutex mutex_; //atomic's mutex
			T object_; //hidden atomic object which methods will be called
			mutable std::condition_variable cv_;
		};
	}
}



#endif //CDPL_CONCURRENT_ATOMIC_H_