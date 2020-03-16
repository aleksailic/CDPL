/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_UTILS_COMMON_H_
#define CDPL_UTILS_COMMON_H_

#ifndef DEBUG_STREAM
#define DEBUG_STREAM stdout
#endif

#include <memory>
#include <string>
#include <cstdio>
#include <mutex>

namespace cdpl {
	namespace utils {
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

		enum class stream_mutex_directive {
			lock = 0,
			unlock = 1
		};

		auto lock = stream_mutex_directive::lock;
		auto unlock = stream_mutex_directive::unlock;

		// use with caution as it can lead to deadlock easily
		std::ostream& operator<<(std::ostream& os, const stream_mutex_directive& directive) {
			static std::mutex stream_mutex;
			if (directive == stream_mutex_directive::lock)
				stream_mutex.lock();
			else if (directive == stream_mutex_directive::unlock)
				stream_mutex.unlock();

			return os;
		}
	}
}

template <typename... Args>
inline void DEBUG_WRITE(const char* name, const char* message, Args... args) {
	auto preformat = cdpl::utils::cstring_format("-- %s: %s --\n", name, message);
	auto debug_string = cdpl::utils::cstring_format(preformat.get(), args...);
	if (DEBUG_STREAM == stdout) std::cout << cdpl::utils::lock;
	fputs(debug_string.get(), DEBUG_STREAM);
	if (DEBUG_STREAM == stdout) std::cout << cdpl::utils::unlock;
}
inline void DEBUG_WRITE(const char* name, const char* message) {
	if (DEBUG_STREAM == stdout) std::cout << cdpl::utils::lock;
	fprintf(DEBUG_STREAM, "-- %s: %s --\n", name, message);
	if (DEBUG_STREAM == stdout) std::cout << cdpl::utils::unlock;
}

#endif //CDPL_UTILS_COMMON_H_
