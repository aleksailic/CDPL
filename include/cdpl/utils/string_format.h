/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_UTILS_STRING_FORMAT_H_
#define CDPL_UTILS_STRING_FORMAT_H_

#include <memory>
#include <string>
#include <cstdio>

namespace cdpl {
	namespace utils {
		template<typename ... Args>
		std::unique_ptr<char[]> cstring_format(const char * format, Args ... args) {
			size_t size = std::snprintf(nullptr, 0, format, args...) + 1; // Extra space for '\0'
			std::unique_ptr<char[]> buf(new char[size]);
			std::snprintf(buf.get(), size, format, args...);
			return buf;
		}

		template<typename ... Args>
		std::string string_format(const std::string& format, Args ... args) {
			size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
			std::unique_ptr<char[]> buf(new char[size]);
			std::snprintf(buf.get(), size, format.c_str(), args...);
			return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
		}
	}
}

#endif //CDPL_UTILS_STRING_FORMAT_H_