/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
 
#ifndef CDPL_LINDA_H_
#define CDPL_LINDA_H_

namespace cdpl {
	namespace linda {
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
		using namespace std::chrono_literals;

		constexpr int random_seed = RANDOM_SEED;
		//if multpiple threadgenerators are started we need to create new seed but in deterministic way
		constexpr int seed_increment = 3691;
		std::atomic<int> seed_offset(0);

		template <class T>
		class ThreadGenerator :public Thread {
			static_assert(std::is_base_of<Thread, T>::value, "T must inherit from Thread");

			struct GeneratorInterface {
				virtual Thread* generate() = 0;
			};
			template <typename... Ts>
			class Generator : public GeneratorInterface {
				std::tuple<Ts...> stored_args;
				std::mutex mutex;
				std::list<Thread*> threads;
				template <std::size_t... Is>
				Thread* helper(std::index_sequence<Is...>) {
					std::unique_lock<std::mutex> lock(mutex);
					threads.push_back(new T(std::get<Is>(stored_args)...));
					return threads.back();
				}
			public:
				Generator(Ts&&... args) : stored_args(std::forward<Ts>(args)...) {}
				~Generator() {
					for (Thread* thread : threads)
						delete thread;
				}
				Thread* generate() {
					return helper(std::index_sequence_for<Ts&&...>());
				}
			};

			std::default_random_engine random_engine;
			std::uniform_int_distribution<uint> random_generator;
			std::shared_ptr<GeneratorInterface> generator;

			void run() {
				while (true) {
					sleep_for(std::chrono::milliseconds(random_generator(random_engine)));
					generator->generate()->start();
				}
			}
		public:
			template <typename... Args>
			ThreadGenerator(std::chrono::duration<double, std::milli> min = 1000ms, std::chrono::duration<double, std::milli> max = 3000ms, Args&&... args)
				: Thread("generator"), random_engine(random_seed + seed_offset), random_generator(min.count(), max.count()),
				generator(new Generator<Args...>(std::forward<Args>(args)...))
			{
				seed_offset += seed_increment;
			}
			ThreadGenerator(const ThreadGenerator& rhs)
				: Thread("generator"), random_engine(random_seed + seed_offset), random_generator(rhs.random_generator), generator(rhs.generator)
			{
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
	}
}

#endif // !CDPL_LINDA_H_
