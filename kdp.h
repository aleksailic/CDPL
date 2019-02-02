#pragma once
#ifndef KDP_H
#define KDP_H

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

namespace Concurrent {
	class Semaphore {
		std::mutex mutex;
		std::condition_variable cond;
		int val;
	public:
		Semaphore(int val = 0) : val(val){}
		inline void signal() {
			std::unique_lock<std::mutex> lock(mutex);
			if (val++ < 0)
				cond.notify_one();
		}
		inline void wait() {
			std::unique_lock<std::mutex> lock(mutex);
			if (--val < 0)
				cond.wait(lock);
		}
	};

	class Mutex {
		Semaphore sem;
	public:
		Mutex() :sem(1) {}
		inline void lock() { sem.wait(); }
		inline void unlock() { sem.signal(); }
	};

	class Thread {
		static void fn_caller(void * arg) {
			static_cast<Thread*>(arg)->run();
		}
		virtual void run() = 0;
		std::thread* thread = nullptr;
		const char * name;
	public:
		Thread(const char * name = "thread") :name(name){}
		void start() {
			thread = new std::thread(Thread::fn_caller, this);
#ifdef DEBUG_THREAD
			std::cout << "THREAD[" << thread->get_id() << "]: " << name << " started" << std::endl;
#endif
		}
		void join() {
			if(thread && thread->joinable()){
#ifdef DEBUG_THREAD
				std::cout << "waiting for " << name << " to join" << std::endl;
#endif
				thread->join();
			}
		}
		virtual ~Thread() {
			join();
#ifdef DEBUG_THREAD
			if(thread){
				auto id = thread->get_id();
				delete thread;
				std::cout << "THREAD[" << id << "]: " << name << " deleted" << std::endl;
			}
#else
			delete thread;
#endif
		}
	};

	typedef Mutex mutex_t;
	typedef Semaphore sem_t;
	typedef unsigned int uint;

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
			struct compare_node_ptr {
				bool operator() (node_t* n1, node_t* n2) {
					return n1->rank < n2->rank;
				}
			};

			mutex_t& monitor_mutex;
			std::priority_queue < node_t*, std::vector<node_t*>, compare_node_ptr > thq;
			std::mutex mutex;
		public:
			cond(mutex_t& monitor_mutex) : monitor_mutex(monitor_mutex) {}
			cond(cond&& rhs) :monitor_mutex(rhs.monitor_mutex) {}
			void wait(uint priority = 0) {
				std::unique_lock<std::mutex> lock(mutex);
				node_t* node = new node_t(priority);
				thq.push(node);
				monitor_mutex.unlock();
				lock.unlock();
#ifdef DEBUG_COND
				fprintf(DEBUG_STREAM, "COND: BLOCK!\n");
#endif
				node->sem->wait();
#ifdef DEBUG_COND
				fprintf(DEBUG_STREAM, "COND: RELEASE!\n");
#endif
				delete node;
				monitor_mutex.lock();
			}
			void signal() {
				std::unique_lock<std::mutex> lock(mutex);
				if (!thq.empty()) {
					thq.top()->sem->signal();
					thq.pop();
				}

			}
			void signalAll() {
				std::unique_lock<std::mutex> lock(mutex);
				while (!thq.empty()) {
					thq.top()->sem->signal();
					thq.pop();
				}
			}
			bool empty() {
				std::unique_lock<std::mutex> lock(mutex);
				return thq.empty();
			}
			bool queue() {
				std::unique_lock<std::mutex> lock(mutex);
				return !thq.empty();
			}
			uint minrank() {
				std::unique_lock<std::mutex> lock(mutex);
				return thq.empty() ? -1 : thq.top()->rank;
			}
		};
		class condition_generator {
			Mutex& monitor_mutex;
		public:
			condition_generator(Mutex& monitor_mutex) :monitor_mutex(monitor_mutex) {}
			constexpr Mutex& operator()() {
				return monitor_mutex;
			}
		};
		condition_generator cond_gen;
	public:
		Monitorable(Mutex& monitor_mutex) :cond_gen(monitor_mutex) {}
	};

	template <class T>
	class Monitor {
		static_assert(std::is_base_of<Monitorable, T>::value, "T must inherit from Monitorable");
	protected:
		Mutex mutex;
		T obj;

		class helper {
			Monitor* mon;
		public:
			helper(Monitor* mon) :mon(mon) {
#ifdef DEBUG_MONITOR
				std::cout << "MONITOR: TRYING TO LOCK" << std::endl;
#endif
				mon->mutex.lock();
#ifdef DEBUG_MONITOR
				std::cout << "MONITOR: LOCKING" << std::endl;
#endif
			}
			~helper() {
				mon->mutex.unlock();
#ifdef DEBUG_MONITOR
				std::cout << "MONITOR: UNLOCKING" << std::endl;
#endif
			}
			T* operator->() { return &mon->obj; }
		};

	public:
		template <typename ...Args>
		Monitor(Args&&... args) :obj(mutex, std::forward<Args>(args)...) {
#ifdef DEBUG_MONITOR
			std::cout << "MONITOR: CREATED" << std::endl;
#endif
		}
		helper operator->() { return helper(this); }
		T& operator*() { return obj; } //return object without monitor
	};

	template<typename T> using monitor = Monitor<T>;
}

namespace MPI {
	using namespace Concurrent;
	typedef uint_fast32_t priority_t;
	typedef uint_fast8_t status_t;

	template <class T>
	class MessageBox {
	public:
		virtual void put(const T& message, priority_t p, long ttl) = 0;
		virtual T get(long ttd, status_t* status) = 0;
	};
	template <class T>
	class CondMessageBox : public MessageBox<T>, public Monitorable {
		cond notFull = cond_gen();
		cond notEmpty = cond_gen();

		uint capacity;

		struct MessageWrap {
			T message;
			priority_t p;
			long ttl;
		};

		std::list<MessageWrap> buffer;
		const char* m_name;
	public:
		CondMessageBox(Mutex& mutex, uint capacity = 10, const char* name = "default") :Monitorable(mutex), capacity(capacity), m_name(name) {}
		void put(const T& message, priority_t p = 0, long ttl = 0) override {
			while (buffer.size() == capacity)
				notFull.wait();
			buffer.push_back(MessageWrap{ message,p,ttl });
			notEmpty.signal();
#ifdef DEBUG_MPI
			std::cout << name() << " message put" << std::endl;
#endif
		}
		T get(long ttd = 0, status_t* status = nullptr) override {
			while (buffer.size() == 0)
				notEmpty.wait();
			T msg = buffer.front().message;
			buffer.pop_front();
			notFull.signal();
#ifdef DEBUG_MPI
			std::cout << name() << " message got" << std::endl;
#endif
			return msg;
		}
		const char* name() { return m_name; }
	};

	template<typename T> using MonitorMessageBox = monitor<CondMessageBox<T>>;
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
				map[typeid(pattern)] = static_cast<void*>(new tm_vec_t<pattern>);
				tm_vec_t<pattern>& tm_vec = *static_cast<tm_vec_t<pattern>*>(map[typeid(pattern)]);
				tm_vec.semaphores.push_back(&m_sem);
#ifdef DEBUG_LINDA
				fprintf(DEBUG_STREAM, "-- linda: locking on %s sem --\n", print_tuple(m_tuple).c_str());
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
							fprintf(DEBUG_STREAM, "-- linda: %s removed --\n", print_tuple(*tuple_iter).c_str());
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
						fprintf(DEBUG_STREAM, "-- linda: locking on %s sem --\n", print_tuple(m_tuple).c_str());
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
			fprintf(DEBUG_STREAM, "-- linda: %s put --\n", print_tuple(tm_vec.tuples.back()).c_str());
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
			:random_engine(random_seed+seed_offset), random_generator(min_seconds, max_seconds), stored_args(std::forward<Args>(args)...) { seed_offset+=seed_increment;}
		~ThreadGenerator(){
			join();
		}
	};


	struct grade_t {
		int passed_num = 0;
		int total_num = 0;

		double score() const {
			return total_num == 0 ? 0 : ((double)passed_num) / total_num;
		}
		friend std::ostream& operator<<(std::ostream& os, const grade_t& grade) {
			return os << grade.passed_num << '/' << grade.total_num<<' '<<grade.score()<<std::endl;
		}
	};

	struct Test {
		virtual void simulate() = 0;
		virtual grade_t grade() { return grade_t(); };
	};

	template <typename ...Args>
	grade_t run_tests(Args&&... args) {
		static const std::size_t size= sizeof...(Args);
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