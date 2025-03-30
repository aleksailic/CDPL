/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_LOG_H_
#define CDPL_LOG_H_

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include "colorize.h"
#include "string_format.h"
#include "stream_lock.h"

#include <string>
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <algorithm>

namespace cdpl {
	namespace log {
#if defined DEBUG || defined ENABLE_LOGGING
		constexpr bool enabled = true;
#else
		constexpr bool enabled = false;
#endif
		// stream that will swallow everything written to it, like /dev/null
		struct null_stream_t : std::streambuf, std::ostream {
			null_stream_t() : std::ostream(this) {}

			int overflow(int c) {
				return 0;
			}
			template <typename T>
			std::ostream& operator<<(const T&) {
				return *this;
			}
		} null_stream;

		struct type {
			using flags = uint_fast8_t;
			enum id {
				debug = 0x01, // Most verbose logs
				info  = 0x02, // General info logs
				warn  = 0x04, // Logs that need attention
				error = 0x08, // Something unexpected happened

				all   = 0x0f  // Helper for all flag selection
			};

			static constexpr const char* name(type::id tid) {
				switch (tid) {
				case type::id::debug: return "DEBUG";
				case type::id::info:  return "INFO";
				case type::id::warn:  return "WARNING";
				case type::id::error: return "ERROR";
				default: throw std::invalid_argument("Bad type id passed");
				}
			}
			std::reference_wrapper<std::ostream> stream = std::ref(null_stream);
			cdpl::utils::terminal_color tc = cdpl::utils::tc::WHITE;
			cdpl::utils::terminal_style ts = cdpl::utils::ts::NORMAL;
		};

		namespace config {
			type::flags default_level = type::id::error; // only errors allowed in log
		}

		struct component {
			using id = std::string;
			static constexpr const std::string& name(const component::id& cid) { return cid; }

			std::reference_wrapper<std::ostream> stream = std::ref(null_stream); // output to another stream as well for more seperation
			type::flags level = config::default_level;
		};

		namespace config {
			// TODO: should be constexpr
			std::unordered_map<type::id, type> types = {
				{type::id::debug, {std::ref(std::cout), cdpl::utils::tc::YELLOW}},
				{type::id::info,  {std::ref(std::cout), cdpl::utils::tc::GREEN}},
				{type::id::warn,  {std::ref(std::cout), cdpl::utils::tc::YELLOW}},
				{type::id::error, {std::ref(std::cerr), cdpl::utils::tc::RED}}
			};
			std::unordered_map<component::id, component> components;
		}

		// per componenet enable/disable 
		void enable(component::id cid, type::flags enabled_types) {
			config::components[cid].level |= enabled_types;
		}
		void disable(component::id cid, type::flags disabled_types) {
			config::components[cid].level &= ~disabled_types;
		}

		// changes default level for all types that are not manually enabled/disabled
		void set_level(type::flags enabled_types) {
			config::default_level = enabled_types;
		}

		void set_stream(type::id tid, std::ostream& stream) {
			config::types[tid].stream = std::ref(stream);
		}
		void set_stream(component::id cid, std::ostream& stream) {
			config::components[cid].stream = std::ref(stream);
		}

		std::string to_upper(std::string string) {
			std::transform(string.begin(), string.end(), string.begin(), ::toupper);
			return string;
		}

		std::string _format(type::id tid, component::id cid, const std::string& message) {
			std::ostringstream oss;
			std::time_t t = std::time(0);
			std::tm* now = std::localtime(&t);

			oss << '[' << std::put_time(now, "%H:%M:%S") << "] "
				<< '[' << to_upper(type::name(tid)) << "] "
				<< '[' << to_upper(component::name(cid)) << "] "
				<< message
				<< '\n';

			return oss.str();
		}

		template <typename... Args>
		void _log(type::id tid, component::id cid, const std::string& format, Args&&... args) {
			auto preformat = _format(tid, cid, format);
			auto message = cdpl::utils::string_format(preformat, std::forward<Args>(args)...);

			// TODO: make mutex for every stream
			config::types[tid].stream.get()
				<< cdpl::utils::lock
				<< cdpl::utils::colorize(
					message, config::types[tid].tc, config::types[tid].ts)
				<< cdpl::utils::unlock;
			config::components[cid].stream.get()
				<< cdpl::utils::lock
				<< cdpl::utils::colorize(
					message, config::types[tid].tc, config::types[tid].ts)
				<< cdpl::utils::unlock;
		}

		template <typename... Args>
		inline void debug(component::id cid, const std::string& format, Args&&... args) {
			_log(type::id::debug, cid, format, std::forward<Args>(args)...);
		}
		template <typename... Args>
		inline void info(component::id cid, const std::string& format, Args&&... args) {
			_log(type::id::info, cid, format, std::forward<Args>(args)...);
		}
		template <typename... Args>
		inline void warn(component::id cid, const std::string& format, Args&&... args) {
			_log(type::id::warn, cid, format, std::forward<Args>(args)...);
		}
		template <typename... Args>
		inline void error(component::id cid, const std::string& format, Args&&... args) {
			_log(type::id::error, cid, format, std::forward<Args>(args)...);
		}
	}
}

#endif //CDPL_LOG_H_