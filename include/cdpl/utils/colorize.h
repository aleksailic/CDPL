/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_UTILS_PRINTING
#define CDPL_UTILS_PRINTING

#include "common.h"

#include <string>

//color coding under windows deps
#ifdef _WIN32
#include <windows.h>
#endif

namespace cdpl {
	namespace utils {
		enum class terminal_color : uint_fast8_t {
#ifdef _WIN32
			BLACK = 0,
			RED = 12,
			GREEN = 10,
			YELLOW = 14,
			BLUE = 9,
			MAGENTA = 13,
			CYAN = 11,
			WHITE = 14
#else
			BLACK = 15,
			RED = 31,
			GREEN = 32,
			YELLOW = 33,
			BLUE = 34,
			MAGENTA = 35,
			CYAN = 36,
			WHITE = 37
#endif
		};

		enum class terminal_style : uint_fast8_t {
			NORMAL = 0,
			RESET = 0,
			BOLD = 1,
			DIM = 2,
			UNDERLINE = 4,
			BLINK = 5,
			INVERSE = 7,
			HIDDEN = 8
		};

		using ts = terminal_style;
		using tc = terminal_color;

		struct terminal_string {
			std::string data;
			terminal_color color = tc::BLACK;
			terminal_style style = ts::NORMAL;
		};

		terminal_string colorize(const std::string& str, terminal_color color, terminal_style style = ts::NORMAL) {
			return terminal_string{ str, color, style };
		}

		std::ostream& operator<<(std::ostream& os, const terminal_string& tstring) {
#ifdef _WIN32
			static HANDLE  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(hConsole, static_cast<uint_fast8_t>(tstring.color));
			os << tstring.data;
			SetConsoleTextAttribute(hConsole, 15);
#else
			os << string_format("\e[%d;%dm%s\e[0m", static_cast<uint_fast8_t>(tstring.style), static_cast<uint_fast8_t>(tstring.color), tstring.data.c_str());
#endif
			return os;
		}
	}
}

#endif //CDPL_UTILS_PRINTING
