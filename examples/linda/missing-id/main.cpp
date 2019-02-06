/*
	This example is part of Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

	..............................................................................

	Missing id problem: Citizen cannot rembember his unique id, although he clearly
	remembers its checksum. Help citizen find his id from improvised Linda database
*/

#define DEBUG_LINDA

#include "CDPL.h"
#include <cstdlib>

using namespace Linda;
using namespace Testbed;

typedef const char * date_t;
typedef int checksum_t; //error correcting checksum, here we just sum all id digits

struct Person {
	const char* name;
	const char* surname;
	long id;
	date_t date;
};

checksum_t get_checksum(long id) {
	int sum = 0;
	for (long iterator = id; iterator; iterator /= 10)
		sum += iterator % 10;
	return sum;
}

int get_matching_digits(long source, long candidate) {
	std::istringstream source_stream, candidate_stream;
	source_stream.str(std::to_string(source));
	candidate_stream.str(std::to_string(candidate));
	char source_c, candidate_c;
	int num_of_matching = 0;
	do {
		source_stream >> source_c;
		candidate_stream >> candidate_c;
		num_of_matching += source_c == candidate_c ? 1 : 0;
	} while (!source_stream.eof() && !candidate_stream.eof());

	return num_of_matching;
}

long find_missing_id(Person& person, checksum_t checksum) {
	std::vector <long> candidates;

	struct {
		long id;
		int num_matched = 0;
	} best_match;

	//get all candidates from our improvised linda database
	long candidate;
	while (rdp(person.name, person.surname, &candidate, person.date)) {
		in(person.name, person.surname, &candidate, person.date);
		candidates.push_back(candidate);

		if (get_matching_digits(person.id, candidate) > best_match.num_matched)
			best_match = { candidate, get_matching_digits(person.id,candidate) };
	}

	//return all candidates that were extrated
	for (auto id : candidates)
		out(person.name, person.surname, id, person.date);

	return get_checksum(best_match.id) == checksum ? best_match.id : -1;
}



// --- SIMMULATION HANDLING PART ---

struct CitizenRecordsTest : public Test {
	void populate() {
		static bool populated = false;
		if (!populated) {
			out("Radomir", "Mihajlovic",(long) 12345678, "13-06-1950");
			out("Vlatko", "Stefanovski",(long) 95261742, "24-01-1957");
			out("Radomir", "Mihajlovic",(long) 12348765, "13-06-1950");
			out("Radomir", "Mihajlovic",(long) 12348567, "13-06-1950");
			out("Radomir", "Mihajlovic",(long) 12348567, "13-06-1949");
			out("Radomir", "Mihajlovic",(long) 11111111, "13-06-1950");
			populated = true;
		}
	}
	void simulate() {
		populate();

		Person test{ "Radomir","Mihajlovic",12345578,"13-06-1950" };

		std::ostringstream oss;
		oss << "MISSING ID IS: "<< find_missing_id(test, 36) << std::endl;
		std::cout << oss.str();
	}
	grade_t grade() {
		populate();
		return run_tests([]{
			Person test{ "Radomir","Mihajlovic",11315131,"13-06-1950" };
			return find_missing_id(test,8) == 11111111;
		},[]{
			Person test{ "Radomir","Mihajlovic",12348567,"13-06-1949" };
			return find_missing_id(test,8) == -1;
		},[]{
			Person test{ "Vlatko","Stefanovski",11111111,"24-01-1957" };
			return find_missing_id(test,36) == 95261742;
		});
	}
};


int main() {
	CitizenRecordsTest test;
	test.simulate();

	grade_t score = test.grade();
	std::cout << "test score: " << score;
	return 0;
}