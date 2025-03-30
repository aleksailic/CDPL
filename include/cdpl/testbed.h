/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_TESTBED_H_
#define CDPL_TESTBED_H_

#ifndef RANDOM_SEED
#define RANDOM_SEED 836939
#endif

#include "concurrent.h"

#include <chrono>
#include <tuple>
#include <list>
#include <random>
#include <memory>
#include <iostream>

namespace cdpl {
	namespace testbed {
		using namespace std::chrono_literals;
		using Thread = cdpl::thread;

		constexpr int random_seed = RANDOM_SEED;
		//if multpiple threadgenerators are started we need to create new seed but in deterministic way
		constexpr int seed_increment = 3691;
		std::atomic<int> seed_offset(0);

		template <class T>
		class ThreadGenerator : public Thread {
			static_assert(std::is_base_of<Thread, T>::value, "T must inherit from Thread");

			struct GeneratorInterface {
				virtual Thread* generate() = 0;
			};
			template <typename... Ts>
			class Generator : public GeneratorInterface {
				std::tuple<Ts...> stored_args;
				cdpl::mutex mutex;
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
			std::uniform_int_distribution<unsigned int> random_generator;
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

#endif // !CDPL_TESTBED_H_
