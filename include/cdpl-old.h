/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_H
#define CDPL_H

//visual studio required
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

//concurrent-deps
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

//typeid-deps
#include <typeindex>
#include <typeinfo>

//container-deps
#include <unordered_map>
#include <vector>
#include <list>
#include <tuple>
#include <queue>

//io deps
#include <iostream>
#include <cstdio>

//misc deps
#include <algorithm>
#include <random>
#include <cstring>
#include <sstream>
#include <utility>
#include <memory>


namespace Concurrent {
	class Monitorable;

	typedef unsigned int uint;

	template <class T>
	class Monitor;

	

	template <class T>
	class Monitor {
		static_assert(std::is_base_of<Monitorable, T>::value, "T must inherit from Monitorable");
	protected:
		mutex_t mutex; //monitor's mutex
		T obj; //hidden Monitorable object which methods will be called

		/**
		 * Helper class that effectively locks and unlocks the mutex on function call.
		 * This is achieved in RAII fashion, as temporary helper object gets constructed
		 * (thus acquiring mutex) when overloaded operator-> gets called, and immediately
		 * after function call gets destroyed (thus releasing mutex).
		 */
		 class Monitorable {
		 protected:
			 class cond {
				 struct node_t {
					 uint rank;
					 sem_t* sem;

					 node_t(uint rank) :rank(rank) {
						 sem = new sem_t(0);
					 }
					 ~node_t() {
						 delete sem;
					 }
				 };
				 //TODO: sorting is not stable by default (STL spec), offer the ability to make it stable
				 struct compare_node_ptr {
					 bool operator() (node_t* n1, node_t* n2) {
						 return n1->rank > n2->rank; //smaller number => higher priority
					 }
				 };

				 mutex_t* & monitor_mutex;
				 std::priority_queue < node_t*, std::vector<node_t*>, compare_node_ptr > thq;
				 std::mutex mutex;
				 const char* name;
			 public:
				 cond(mutex_t* & monitor_mutex, const char * name = "") : monitor_mutex(monitor_mutex), name(name) {
#ifdef DEBUG_COND
					 DEBUG_WRITE("condition %s", "created", name);
#endif
				 }
				 cond(cond&& rhs) :monitor_mutex(rhs.monitor_mutex) {}
				 /**
				  *  Blocks the current process until the condition variable is woken up.
				  *  @param uint priority Set blocked process' priority in internal blocked queue. Smaller number => higher priority.
				  */
				 void wait(uint priority = 0) {
					 std::unique_lock<std::mutex> lock(mutex);
					 node_t* node = new node_t(priority);
					 thq.push(node);
					 //TODO: discuss whether to add check for nullptr in case user creates bare monitorable object and calls wait on cv
					 monitor_mutex->unlock();
					 lock.unlock();
#ifdef DEBUG_COND
					 auto descriptor = Thread::get_descriptor(std::this_thread::get_id());
					 DEBUG_WRITE("condition %s", "blocked thread[#%d] %s", name, descriptor.id, descriptor.name);
#endif
					 node->sem->wait();
#ifdef DEBUG_COND
					 DEBUG_WRITE("condition %s", "released thread[#%d] %s", name, descriptor.id, descriptor.name);
#endif
					 delete node;
					 monitor_mutex->lock();
				 }
				 /**
				  * Unblock process with the highest priority from blocked queue
				  * Smaller number => higher priority.
				  */
				 void signal() {
					 std::unique_lock<std::mutex> lock(mutex);
					 if (!thq.empty()) {
						 thq.top()->sem->signal();
						 thq.pop();
					 }

				 }
				 /**
				  * Unblock all processes from blocked queue
				  */
				 void signalAll() {
					 std::unique_lock<std::mutex> lock(mutex);
					 while (!thq.empty()) {
						 thq.top()->sem->signal();
						 thq.pop();
					 }
				 }
				 /**
				  * Check whether blocked queue is empty
				  * @return bool
				  */
				 bool empty() {
					 std::unique_lock<std::mutex> lock(mutex);
					 return thq.empty();
				 }
				 /**
				  *  Check if there are processes in blocked queue
				  * @return bool
				  */
				 bool queue() {
					 std::unique_lock<std::mutex> lock(mutex);
					 return !thq.empty();
				 }
				 /**
				  * Get the priority of the next process to be unblocked from queue.
				  * If queue is empty returns MAXUINT
				  * @return uint
				  */
				 uint minrank() {
					 std::unique_lock<std::mutex> lock(mutex);
					 return thq.empty() ? -1 : thq.top()->rank;
				 }
			 };
			 /**
			  * Each Monitorable object has its own condition_generator that
			  * contains reference to its mutex and serves to deliver it to
			  * condition variables when they are constructed
			  */
			 class condition_generator {
				 mutex_t* monitor_mutex = nullptr;
				 void set_mutex(mutex_t* mutex) {
					 monitor_mutex = mutex;
				 }
			 public:
				 cond operator()(const char* name = "") {
					 return cond(monitor_mutex, name);
				 }
				 template<class T>
				 friend class Monitor;
			 };
			 condition_generator cond_gen;
		 public:
			 template<class T>
			 friend class Monitor;
		 };
		class helper {
			Monitor* mon;
		public:
			helper(Monitor* mon) :mon(mon) {
				cdpl::log::info()
#ifdef DEBUG_MONITOR
				DEBUG_WRITE("monitor", "trying to lock");
#endif
				mon->mutex.lock();
#ifdef DEBUG_MONITOR
				DEBUG_WRITE("monitor", "locking");
#endif
			}
			~helper() {
				mon->mutex.unlock();
#ifdef DEBUG_MONITOR
				DEBUG_WRITE("monitor", "unlocking");
#endif
			}
			/**
			 * Nifty hack to insert underlying object and create illusion of direct access
			 */
			T* operator->() { return &mon->obj; }
		};

	public:
		template <typename ...Args>
		Monitor(Args&&... args) :obj(std::forward<Args>(args)...) {
			static_cast<Monitorable&>(obj).cond_gen.set_mutex(&mutex);
			if constexpr (cdpl::debug::enabled) cdpl::log::debug("monitor", "created");
		}
		/**
		 * Call underlying object's function in mutually exclusive way.
		 */
		helper operator->() { return helper(this); }
		/**
		 * Returns demonitorized object. Use with caution as locking on condition
		 * from underlying Monitorable object will lead to deadlock.
		 */
		T& operator*() { return obj; }
	};

	template<typename T> using monitor = Monitor<T>;
}




#endif
