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

#ifndef RANDOM_SEED
#define RANDOM_SEED 836939
#endif

#ifndef DEBUG_STREAM
#define DEBUG_STREAM stdout
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

//color coding under windows deps
#ifdef _WIN32
#include <windows.h>
#endif

template <typename... Args>
inline void DEBUG_WRITE(const char* name, const char* format, Args&&... args);
inline void DEBUG_WRITE(const char* name, const char* format);

namespace Concurrent {
	class Semaphore {
		std::mutex mutex;
		std::condition_variable cond;
		int val;
	public:
		Semaphore(int val = 0) : val(val) {}
		/**
		 * Increments the internal value of semaphore by 1.
		 * If the pre-increment value was negative (there are processes waiting for resource),
		 * it transfers a blocked process from the semaphore's waiting queue to the ready queue
		 */
		inline void signal() {
			std::unique_lock<std::mutex> lock(mutex);
			if (val++ < 0)
				cond.notify_one();
		}
		/**
		 * Decrements the internal value of semaphore by 1. After the decrement,
		 * if the post-decrement value was negative, the process executing wait is blocked
		 * (added to the semaphore's queue). Otherwise, the process continues execution.
		 */
		inline void wait() {
			std::unique_lock<std::mutex> lock(mutex);
			if (--val < 0)
				cond.wait(lock);
		}
	};
	typedef Semaphore sem_t;

	/**
	 * Eqiuivalent of semaphore with initial value 1, where lock and unlock methods
	 * are aliases for signal and wait operation.
	 */
	class Mutex {
		sem_t sem;
	public:
		Mutex() :sem(1) {}
		inline void lock() { sem.wait(); }
		inline void unlock() { sem.signal(); }
	};

	typedef Mutex mutex_t;
};

namespace Utils {
	template<typename ... Args>
	std::unique_ptr<char[]> cstring_format(const char * format, Args ... args) {
		size_t size = snprintf(nullptr, 0, format, args ...) + 1; // Extra space for '\0'
		std::unique_ptr<char[]> buf(new char[size]);
		snprintf(buf.get(), size, format, args ...);
		return buf;
	}

	template<typename ... Args>
	std::string string_format(const std::string& format, Args ... args) {
		size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
		std::unique_ptr<char[]> buf(new char[size]);
		snprintf(buf.get(), size, format.c_str(), args ...);
		return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
	}


#ifdef _WIN32
	enum class TerminalColor : uint_fast8_t {
		BLACK = 0,
		RED = 12,
		GREEN = 10,
		YELLOW = 14,
		BLUE = 9,
		MAGENTA = 13,
		CYAN = 11,
		WHITE = 14
	};
#else 
	enum class TerminalColor : uint_fast8_t {
		BLACK = 15,
		RED = 31,
		GREEN = 32,
		YELLOW = 33,
		BLUE = 34,
		MAGENTA = 35,
		CYAN = 36,
		WHITE = 37
	};
#endif
	enum class TerminalStyle : uint_fast8_t {
		NORMAL = 0,
		RESET = 0,
		BOLD = 1,
		DIM = 2,
		UNDERLINE = 4,
		BLINK = 5,
		INVERSE = 7,
		HIDDEN = 8
	};
	typedef TerminalStyle TS;
	typedef TerminalColor TC;

	struct TerminalString {
		std::string string;
		TerminalColor color;
	};

	TerminalString colorize(const std::string& str, TerminalColor color, TerminalStyle style = TerminalStyle::NORMAL) {
#ifdef _WIN32
		return TerminalString{ str, color };
#else
		return TerminalString{ string_format("\e[%d;%dm%s\e[0m", static_cast<uint_fast8_t>(style), static_cast<uint_fast8_t>(color), str.c_str()) };
#endif
	}
	std::ostream& operator<<(std::ostream&os, const TerminalString& wcs) {
#ifdef _WIN32
		static HANDLE  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hConsole, static_cast<uint_fast8_t>(wcs.color));
		os << wcs.string;
		SetConsoleTextAttribute(hConsole, 15);
		return os;
#else
		return os << wcs.string;
#endif
	}

	enum class PrintMutexDirective {
		LOCK = 0,
		UNLOCK = 1
	};
	auto lock = PrintMutexDirective::LOCK;
	auto unlock = PrintMutexDirective::UNLOCK;

	std::ostream& operator<<(std::ostream& os, const PrintMutexDirective& directive) {
		static Concurrent::mutex_t print_mutex;
		if (directive == PrintMutexDirective::LOCK)
			print_mutex.lock();
		else if (directive == PrintMutexDirective::UNLOCK)
			print_mutex.unlock();

		return os;
	}
}

template <typename... Args>
inline void DEBUG_WRITE(const char* name, const char* format, Args&&... args) {
	auto preformat = Utils::cstring_format("-- %s: %s --\n", name, format);
	auto debug_string = Utils::cstring_format(preformat.get(), std::forward<Args>(args)...);
	if (DEBUG_STREAM == stdout) std::cout << Utils::lock;
	fputs(debug_string.get(), DEBUG_STREAM);
	if (DEBUG_STREAM == stdout) std::cout << Utils::unlock;
}
inline void DEBUG_WRITE(const char* name, const char* format) {
	if (DEBUG_STREAM == stdout) std::cout << Utils::lock;
	fprintf(DEBUG_STREAM, "-- %s: %s --\n", name, format);
	if (DEBUG_STREAM == stdout) std::cout << Utils::unlock;
}

namespace Concurrent {
	class Monitorable;
	class Thread {
		typedef struct {
			int id;
			const char* name;
		} descriptor_t;

		virtual void run() = 0;
		static void fn_caller(void * arg) {
			static_cast<Thread*>(arg)->run();
		}

		std::thread* thread = nullptr;
		descriptor_t descriptor;

		static std::atomic<int> next_id;
		static std::mutex thread_map_mutex;
		static std::unordered_map<std::thread::id, descriptor_t> std_thread_map;
	protected:
		// sleep_for alias that shall be used only from inside function so always targets this_thread
		template< class Rep, class Period>
		void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration) {
			std::this_thread::sleep_for(sleep_duration);
		}
		// allow the user to change thread's name whenever he wants as it is useful for debugging
		void set_name(const char* name) {
			descriptor.name = name;
			if (thread) {
				std::unique_lock<std::mutex> lock(thread_map_mutex);
				std_thread_map[thread->get_id()] = descriptor;
			}
		}
		descriptor_t get_descriptor() const {
			return descriptor;
		}
		//needed for condition printing, TODO: consider putting in private to hide inner implementaiton
		static descriptor_t get_descriptor(std::thread::id std_thread_id) {
			std::unique_lock<std::mutex> lock(thread_map_mutex);
			return std_thread_map[std::this_thread::get_id()];
		}
		friend class Monitorable;
	public:
		Thread(const char* name = "") :descriptor({ next_id++,name }) {}
		void start() {
			thread = new std::thread(Thread::fn_caller, this);
			std::unique_lock<std::mutex> lock(thread_map_mutex);
			std_thread_map[thread->get_id()] = descriptor;
#ifdef DEBUG_THREAD
			DEBUG_WRITE("thread[#%d] %s", "started", descriptor.id, descriptor.name);
#endif
		}
		void join() {
			if (thread && thread->joinable()) {
#ifdef DEBUG_THREAD
				DEBUG_WRITE("thread[#%d] %s", "joined", descriptor.id, descriptor.name);
#endif
				thread->join();
			}
		}
		virtual ~Thread() {
			//join() removed. Library shouldn't enforce this and can create seg. faults
			if (thread) {
				delete thread;
#ifdef DEBUG_THREAD
				DEBUG_WRITE("thread[#%d] %s", "deleted", descriptor.id, descriptor.name);
#endif
			}
		}
	};
	std::atomic<int> Thread::next_id{ 0 };
	std::mutex Thread::thread_map_mutex{};
	std::unordered_map<std::thread::id, Thread::descriptor_t> Thread::std_thread_map{};

	typedef unsigned int uint;

	template <class T>
	class Monitor;

	/**
	 * Parent class for all classes that are candidates to become monitor
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
		class helper {
			Monitor* mon;
		public:
			helper(Monitor* mon) :mon(mon) {
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
#ifdef DEBUG_MONITOR
			DEBUG_WRITE("monitor", "created");
#endif
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

namespace MPI {
	using namespace std::chrono_literals;
	typedef uint_fast32_t priority_t;
	typedef uint_fast8_t status_t;
	typedef unsigned int uint;
	typedef std::chrono::duration<long, std::milli> duration_t;
	typedef std::chrono::time_point< std::chrono::high_resolution_clock > timestamp_t;

	enum { VERY_HIGH, HIGH, MEDIUM, LOW, VERY_LOW };
	enum { SUCCESS, TIMEOUT, EXPIRED };

	template <class T>
	class MessageBox {
	public:
		virtual void put(const T& message, priority_t p, const duration_t& ttl) = 0;
		virtual T get(const duration_t& ttd, status_t* status) = 0;
	};
	template <class T>
	class MonitorableMessageBox : public MessageBox<T>, public Concurrent::Monitorable {
		cond notFull = cond_gen();
		cond notEmpty = cond_gen();

		uint capacity;

		struct MessageWrap {
			T message;
			priority_t p;
			duration_t ttl;
			timestamp_t ts;
			MessageWrap(const T& message, priority_t p, const duration_t& ttl) :message(message), p(p), ttl(ttl), ts(std::chrono::high_resolution_clock::now()) {}
			friend bool operator>(const MessageWrap& mw1, const MessageWrap& mw2) { return mw1.p > mw2.p; }
		};

		typedef std::priority_queue<MessageWrap, std::vector<MessageWrap>, std::greater<MessageWrap>> buffer_t;
		buffer_t buffer;
		const char* m_name;
	public:
		MonitorableMessageBox(uint capacity = 10, const char* name = "") : capacity(capacity), m_name(name) {}
		void put(const T& message, priority_t p = MEDIUM, const duration_t& ttl = 0ms) override {
			while (buffer.size() == capacity)
				notFull.wait();
			buffer.emplace(message, p, ttl);
			notEmpty.signal();
#ifdef DEBUG_MPI
			DEBUG_WRITE("MessageBox(%s)", "message put", m_name);
#endif
		}
		T get(const duration_t& ttd = 0ms, status_t* status = nullptr) override {
			auto canContinue = std::make_shared<cond>(cond(cond_gen()));
			if (ttd != 0ms) {
				std::thread([](std::shared_ptr<cond> canContinue, const duration_t& ttd) {
					std::this_thread::sleep_for(ttd);
					//TODO: discuss whether to add safety check that user has locked on canContinue before reaching this step (only possible if ttd is ridiculously small)
					canContinue->signal();
				}, canContinue, ttd).detach();
			}
			//safe to use rawe pointers because if monitor gets destroyed unwantingly, this won't be released anyway so there will be memory leak
			//TODO: discuss whether to save the user from leakage
			std::thread([](std::shared_ptr<cond> canContinue, buffer_t* buffer, cond* notEmpty) {
				while (buffer->size() == 0)
					notEmpty->wait();
				canContinue->signal();
			}, canContinue, &buffer, &notEmpty).detach();

			canContinue->wait();
			if (buffer.size()) {
				auto msg_wrap = buffer.top();
				buffer.pop();
				notFull.signal();

				if (msg_wrap.ttl == 0ms || std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - msg_wrap.ts).count() > 0) {
#ifdef DEBUG_MPI
					DEBUG_WRITE("MessageBox(%s)", "message recieved", m_name);
#endif
					if (status) *status = SUCCESS;
					return msg_wrap.message;
				}
				else {
#ifdef DEBUG_MPI
					DEBUG_WRITE("MessageBox(%s)", "message expired", m_name);
#endif
					if (status) *status = EXPIRED;
				}
			}
			else { //released on timeout
#ifdef DEBUG_MPI
				DEBUG_WRITE("MessageBox(%s)", "message timed out", m_name);
#endif
				if (status) *status = TIMEOUT;
			}
			return T(); //TODO: discuss whether it is better to enforce the user to overload cast operator or to have specific constructor if this fails or to leave it like this
		}
		const char* name() const { return m_name; }
	};

	template<typename T> using MonitorMessageBox = Concurrent::monitor<MonitorableMessageBox<T>>;
}

namespace Linda {
	std::unordered_map<std::type_index, void*> map;

	template <typename T>
	class Active {
		virtual T run() = 0;
	public:
		typedef T type;
	};

	namespace impl {
		using namespace Concurrent;
		mutex_t map_mutex;

		template <typename T>
		struct tm_vec_t {
			mutex_t mutex;
			std::vector<T> tuples;
			std::vector<sem_t*> semaphores;
		};

		enum on_found_t { NOTHING, REMOVE };
		enum on_notfound_t { REPEAT, RETURN };

		template <typename ...Args>
		using tuple_cat_t = decltype(std::tuple_cat(std::declval<Args>()...));

		template <typename T, typename P, typename F, typename R = void*, std::size_t ...Indices>
		void tuple_pattern_cmp_impl(T&& tuple, P&& pattern, F&& fn, R* result, std::index_sequence<Indices...>) {
			using swallow = int[];
			(void)swallow {
				1,
					(fn(std::get<Indices>(std::forward<T>(tuple)), std::get<Indices>(std::forward<P>(pattern)), result), void(), int{})...
			};
		}
		template <typename T, typename P, typename F, typename R = void>
		void tuple_pattern_cmp(T&& tuple, P&& pattern, F fn, R* result = nullptr) {
			constexpr std::size_t N = std::tuple_size<std::remove_reference_t<T>>::value;
			tuple_pattern_cmp_impl(
				std::forward<T>(tuple), std::forward<P>(pattern), std::forward<F>(fn), result, std::make_index_sequence<N>{}
			);
		}

		template <typename T, typename F, typename R = void*, std::size_t ...Indices>
		void tuple_for_each_impl(T&& tuple, F&& fn, R* result, std::index_sequence<Indices...>) {
			using swallow = int[];
			(void)swallow {
				1,
					(fn(std::get<Indices>(std::forward<T>(tuple)), result), void(), int{})...
			};
		}
		template <typename T, typename F, typename R = void>
		void tuple_for_each(T&& tuple, F fn, R* result = nullptr) {
			constexpr std::size_t N = std::tuple_size<std::remove_reference_t<T>>::value;
			tuple_for_each_impl(
				std::forward<T>(tuple), std::forward<F>(fn), result, std::make_index_sequence<N>{}
			);
		}

		struct is_eq_f {
			//only compare the same type!
			template <typename T,
				std::enable_if_t<!std::is_same<T, const char *>::value>* = nullptr>
				inline void operator()(T t1, T t2, bool* result) {
				if (t1 != t2)
					*result = false;
			}
			template <typename T,
				std::enable_if_t<std::is_same<T, const char *>::value>* = nullptr>
				inline void operator()(T t1, T t2, bool* result) {
				if (strcmp(t1, t2) != 0)
					*result = false;
			}
			//types are different so don't do comparison
			template <typename T>
			inline void operator()(T t1, T* t2, bool* result) {}
		};

		template <typename T, typename P>
		bool is_eq(T&& tuple, P&& pattern) {
			bool result = true;
			tuple_pattern_cmp(std::forward<T>(tuple), std::forward<P>(pattern), is_eq_f(), &result);
			return result;
		}

		struct ch_ptr_vals_f {
			template<typename T1, typename T2, std::enable_if_t< !(std::is_same<T1, char*>::value && std::is_same<T2, const char*>::value)>* = nullptr>
			inline void operator()(T1 t1, T2 t2, void* r) {
				static_assert(std::is_same<T1, T2>::value, "Irregularity with pattern, it should differ only by pointer.T1 and T2 are different classes");
			}
			//types are the same, do nothing
			template <typename T, std::enable_if_t< !std::is_same<T, const char*>::value>* = nullptr >
			inline void operator()(T t1, T t2, void* r) {}
			template <typename T>
			inline void operator()(T t1, T* t2, void* r) { *t2 = t1; }
			//const char * makes problems so just specialize it
			template <typename T1, typename T2, std::enable_if_t< std::is_same<T1, char*>::value && std::is_same<T2, const char*>::value>* = nullptr >
			inline void operator()(char* t1, const char* t2, void*) { strcpy(t1, t2); }
		};
		template <typename T, typename P>
		void ch_ptr_vals(T& tuple, P& pattern) {
			tuple_pattern_cmp(tuple, pattern, ch_ptr_vals_f());
		}

		struct print_f {
			template <typename T,
				typename std::enable_if_t< !std::is_pointer<std::remove_reference_t<T>>::value || std::is_same<std::remove_reference_t<T>, const char *>::value>* = nullptr>
				inline void operator()(T&& elem, std::ostringstream* oss) {
				*oss << elem << ',';
			}
			template <typename T,
				typename std::enable_if_t< std::is_pointer<std::remove_reference_t<T>>::value && !std::is_same<std::remove_reference_t<T>, const char *>::value>* = nullptr>
				inline void operator()(T&& elem, std::ostringstream* oss) {
				*oss << '?' << ',';
			}
		};
		template <typename T>
		std::string print_tuple(T&& tuple) {
			std::ostringstream oss;
			oss << '(';
			tuple_for_each(std::forward<T>(tuple), print_f(), &oss);
			std::string str = oss.str();
			str.pop_back();
			str.push_back(')');
			return str;
		}

		template <typename ...Args>
		inline bool in(on_found_t found_action, on_notfound_t notfound_action, Args... args) {
			using tuple_t = std::tuple<Args...>;
			using strip_ptr_tuple_t = tuple_cat_t<
				typename std::conditional<
				std::is_pointer<Args>::value && !std::is_same<const char*, Args>::value,
				std::tuple<typename std::remove_pointer<Args>::type >,
				std::tuple<Args>
				>::type...
			>;
			using pattern = strip_ptr_tuple_t;

			sem_t m_sem;
			tuple_t m_tuple(std::forward<Args>(args)...);
			bool found = false;

			map_mutex.lock();
			if (!map[typeid(pattern)]) {
				if (notfound_action == RETURN) {
					map_mutex.unlock();
					return false;
				}
				map[typeid(pattern)] = static_cast<void*>(new tm_vec_t<pattern>);
				tm_vec_t<pattern>& tm_vec = *static_cast<tm_vec_t<pattern>*>(map[typeid(pattern)]);
				tm_vec.semaphores.push_back(&m_sem);
#ifdef DEBUG_LINDA
				DEBUG_WRITE("linda", "locking on %s sem", print_tuple(m_tuple).c_str());
#endif
				map_mutex.unlock();
				m_sem.wait();
			}
			else {
				map_mutex.unlock();
			}
			tm_vec_t<pattern>& tm_vec = *static_cast<tm_vec_t<pattern>*>(map[typeid(pattern)]);
			while (!found) {
				tm_vec.mutex.lock(); //only one process can acces vector!
				for (auto tuple_iter = tm_vec.tuples.begin(); tuple_iter != tm_vec.tuples.end(); tuple_iter++) {
					if (is_eq(*tuple_iter, m_tuple)) {
						ch_ptr_vals(*tuple_iter, m_tuple);
						found = true;
						if (found_action == REMOVE) {
#ifdef DEBUG_LINDA
							DEBUG_WRITE("linda", "%s removed", print_tuple(*tuple_iter).c_str());
#endif
							tm_vec.tuples.erase(tuple_iter);
						}
						break;
					}
				}
				if (!found) {
					if (notfound_action == REPEAT) {
						tm_vec.semaphores.push_back(&m_sem);
						tm_vec.mutex.unlock();
#ifdef DEBUG_LINDA
						DEBUG_WRITE("linda", "locking on %s sem", print_tuple(m_tuple).c_str());
#endif
						m_sem.wait(); //lock on sem and wait
					}
					else if (notfound_action == RETURN) {
						break;
					}
				}
			}
			tm_vec.mutex.unlock();
			return found;
		}

		template<typename... Ts>
		struct is_std_tuple : std::false_type {};
		template<typename... Ts>
		struct is_std_tuple<std::tuple<Ts...>> : std::true_type {};

		template <typename ...Ts>
		inline void out(std::tuple<Ts...>&& tuple) {
			using tuple_t = std::tuple<Ts...>;

			map_mutex.lock();
			if (!map[typeid(tuple_t)])
				map[typeid(tuple_t)] = static_cast<void*>(new tm_vec_t<tuple_t>);
			map_mutex.unlock();

			tm_vec_t<tuple_t>& tm_vec = *static_cast<tm_vec_t<tuple_t>*>(map[typeid(tuple_t)]);
			tm_vec.mutex.lock(); //lock tm_vec mutex so only this process can use it for critical operation
			tm_vec.tuples.push_back(std::forward<tuple_t>(tuple));

#ifdef DEBUG_LINDA
			DEBUG_WRITE("linda", "%s put", print_tuple(tm_vec.tuples.back()).c_str());
#endif
			//unlock and erase all semaphores
			for (sem_t* sem : tm_vec.semaphores) {
				sem->signal();
			}
			tm_vec.semaphores.clear();

			tm_vec.mutex.unlock(); //release mutex for others
		}

		template <typename ...Args,
			std::enable_if_t< !is_std_tuple< Args... >::value >* = nullptr
		>
			inline void out(Args... args) {
			using tuple_t = std::tuple<Args...>;
			out(tuple_t(std::forward<Args>(args)...));
		}


		template < template <typename...> class base, typename derived>
		struct is_base_of_template_impl {
			template<typename... Ts>
			static constexpr std::true_type  test(const base<Ts...> *);
			static constexpr std::false_type test(...);
			using type = decltype(test(std::declval<derived*>()));
		};

		template < template <typename...> class base, typename derived>
		using is_base_of_template = typename is_base_of_template_impl<base, derived>::type;


		template <typename T> struct default_type { using type = T; };
		template <typename T> struct active_type { using type = typename T::Active::type; };

		template <typename Arg>
		using return_type = typename std::conditional_t<
			is_base_of_template< Active, Arg>::value, //checking if derived class of Active passed
			active_type<Arg>,
			typename std::conditional_t<
			std::is_pointer<Arg>::value && std::is_function<typename std::remove_pointer<Arg>::type>::value, //checking if function pointer passed
			std::result_of<Arg && ()>,
			default_type<Arg>
			>
		>::type;


		//on derived class of Ative use the run method  

		template <typename T, std::size_t... Indices>
		auto vec_to_tuple_impl(std::vector<void*>& v, T pattern, std::index_sequence<Indices...>) {
			return std::make_tuple(
				*static_cast<typename std::tuple_element<Indices, T>::type *> (v[Indices])...
			);
		}

		template <typename T>
		auto vec_to_tuple(std::vector<void*>& v, T&& pattern) {
			constexpr std::size_t N = std::tuple_size<std::remove_reference_t<T>>::value;
			return vec_to_tuple_impl(v, pattern, std::make_index_sequence<N>());
		}

		struct run_active_tuple_f {
			//if pattern and tuple are the same just put element from tuple
			template <typename T>
			inline void operator()(T& elem, T type, std::vector<void*>* elements) {
				elements->push_back(
					static_cast<void*>(new T(elem))
				);
			}

			// if function pointer passed
			template <typename Fn, typename T,
				std::enable_if_t<std::is_pointer<Fn>::value && std::is_function<typename std::remove_pointer<Fn>::type>::value, T>* = nullptr>
				inline void operator()(Fn fn, T type, std::vector<void*>* elements) {
				elements->push_back(
					static_cast<void*>(new T(fn()))
				);
			}

			template <typename A, typename T,
				std::enable_if_t<!std::is_pointer<A>::value, T>* = nullptr>
				inline void operator()(A& active, T type, std::vector<void*>* elements) {
				static_assert(is_base_of_template<Active, A>::value, "Object passed must inherit from Active");
				elements->push_back(
					static_cast<void*>(new T(active.run()))
				);
			}
		};

		template <typename T, typename P, typename V>
		void run_active_tuple(T&& tuple, P&& pattern, V* tm_vec) {
			std::vector<void*> elements;
			tuple_pattern_cmp(tuple, pattern, run_active_tuple_f(), &elements);

			out(vec_to_tuple(elements, pattern));

			auto vec_eraser = std::pair< std::vector<void*>&, int >(elements, 0);
			tuple_for_each(pattern, [](auto element, auto vec_eraser) {
				delete static_cast<decltype(element)*>(vec_eraser->first[vec_eraser->second++]);
			}, &vec_eraser);
		}

		template <typename ...Args>
		inline void eval(Args... args) {
			using tuple_t = std::tuple<Args...>;
			using pattern_t = tuple_cat_t<std::tuple<return_type<Args>>...>;

			map_mutex.lock();
			if (!map[typeid(pattern_t)])
				map[typeid(pattern_t)] = static_cast<void*>(new tm_vec_t<pattern_t>);
			map_mutex.unlock();

			std::thread m_thread(
				run_active_tuple<tuple_t, pattern_t, tm_vec_t<pattern_t> >,
				tuple_t(std::forward<Args>(args)...),
				pattern_t(),
				static_cast<tm_vec_t<pattern_t>*>(map[typeid(pattern_t)])
			);

			m_thread.detach();
		}

	}

	template <typename ...Args>
	void out(Args&&... args) {
		impl::out(std::forward<Args>(args)...);
	}

	template <typename ...Args>
	void rd(Args&&... args) {
		impl::in(impl::on_found_t::NOTHING, impl::on_notfound_t::REPEAT, std::forward<Args>(args)...);
	}

	template <typename ...Args>
	bool rdp(Args&&... args) {
		return impl::in(impl::on_found_t::NOTHING, impl::on_notfound_t::RETURN, std::forward<Args>(args)...);
	}

	template <typename ...Args>
	void in(Args&&... args) {
		impl::in(impl::on_found_t::REMOVE, impl::on_notfound_t::REPEAT, std::forward<Args>(args)...);
	}

	template <typename ...Args>
	bool inp(Args&&... args) {
		return impl::in(impl::on_found_t::REMOVE, impl::on_notfound_t::RETURN, std::forward<Args>(args)...);
	}

	template <typename ...Args>
	void eval(Args&&... args) {
		impl::eval(std::forward<Args>(args)...);
	}
};
namespace Testbed {
	using namespace Concurrent;
	constexpr int random_seed = RANDOM_SEED;
	constexpr int seed_increment = 3691;
	std::atomic<int> seed_offset(0); //if multpiple threadgenerators are started we need to create new seed but in deterministic way

	template <class T, typename ...Args>
	class ThreadGenerator :public Thread {
		static_assert(std::is_base_of<Thread, T>::value, "T must inherit from Thread");
		std::default_random_engine random_engine;
		std::uniform_int_distribution<uint> random_generator;
		std::tuple<Args&&...> stored_args;

		//helper function for generating threads from stored arguments, really hacky stuff...
		template<std::size_t... Is>
		Thread* generateThread(const std::tuple<Args&&...>& tuple,
			std::index_sequence<Is...>) {
			return static_cast<Thread*>(new T(std::get<Is>(tuple)...));
		}

		void run() {
			while (true) {
				std::this_thread::sleep_for(std::chrono::seconds(random_generator(random_engine)));
				generateThread(stored_args, std::index_sequence_for<Args&&...>())->start();
			}
		}
	public:
		ThreadGenerator(uint min_seconds = 1, uint max_seconds = 3, Args&&... args)
			:Thread("generator"), random_engine(random_seed + seed_offset), random_generator(min_seconds, max_seconds), stored_args(std::forward<Args>(args)...) {
			seed_offset += seed_increment;
		}
		~ThreadGenerator() {
			join();
		}
	};


	struct grade_t {
		int passed_num = 0;
		int total_num = 0;

		double score() const {
			return total_num == 0 ? 0 : ((double)passed_num) / total_num * 100;
		}
		friend std::ostream& operator<<(std::ostream& os, const grade_t& grade) {
			return os << grade.passed_num << '/' << grade.total_num << ' ' << grade.score() << '%' << std::endl;
		}
	};

	struct Test {
		virtual void simulate() = 0;
		virtual grade_t grade() { return grade_t(); };
	};

	template <typename ...Args>
	grade_t run_tests(Args&&... args) {
		static const std::size_t size = sizeof...(Args);
		bool results[size] = { args()... };
		grade_t grade;
		for (std::size_t i = 0; i < size; i++) {
			grade.total_num++;
			grade.passed_num += results[i] ? 1 : 0;
		}
		return grade;
	}
};
#endif
