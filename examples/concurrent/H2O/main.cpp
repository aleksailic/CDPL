/*
	This example is part of Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

	..............................................................................

	Forming of H2O Mollecule problem: Atoms arrive at barrier at random time. Only
	when there are 2H atoms and 1O atom can water mollecule be formed.

	Solution given using barrier monitor.
*/

#include "CDPL.h"
#include <exception>
#include <cstdlib>

using namespace Concurrent;
using namespace Testbed;
using namespace Utils;

struct Atom: public Thread{
	enum type_t {H,O};
	type_t type;
	void run() override;

	Atom(type_t type): type(type), Thread(type == H ? "H" : "O") { }
};
class Barrier:public Monitorable{
	cond enough_hydrogen = cond_gen();
	cond enough_oxygen = cond_gen();

	cond hydrogen_buffer = cond_gen();
	cond oxygen_buffer = cond_gen();

	int h_count = 0;
	int o_count = 0;
	int form_count = 0;
public:
	void insert(Atom& atom){
		if(atom.type == Atom::H){
			h_count++;
			if(h_count > 2){
				hydrogen_buffer.wait();
				h_count++; //as it will be reset when new element is formed
			}

			if(h_count==1)
				enough_hydrogen.wait();
			else
				enough_hydrogen.signalAll();

			if(o_count==0)
				enough_oxygen.wait();

		}else if(atom.type == Atom::O){
			o_count++;
			if(o_count > 1){
				oxygen_buffer.wait();
				o_count++; //as it will be reset when new element is formed
			}
			enough_oxygen.signalAll();

			if(h_count != 2)
				enough_hydrogen.wait();
		}else{
			throw std::exception();
		}

		form(atom);
	}
	void form(Atom& atom){
		std::cout << lock;

		if(form_count==0)
			std::cout << "Molecule is forming: ";
		std::cout << (atom.type == Atom::H ? 'H' : 'O') <<" ";

		if(++form_count==3){
			std::cout << std::endl << colorize("Molecule H2O formed!",TC::GREEN) << std::endl;
			form_count = 0;
			h_count = 0;
			o_count = 0;

			//wake 3 needed atoms if possible
			hydrogen_buffer.signal();
			hydrogen_buffer.signal();
			oxygen_buffer.signal();
		}

		std::cout << unlock;
	}
};

static monitor<Barrier> mon;
void Atom::run(){
	std::cout << lock << "ATOM " << colorize(type == H ? "H" : "O", TC::YELLOW) << " created\n" << unlock; 
	mon->insert(*this);
}

int main(){
	srand(random_seed);
	std::vector<ThreadGenerator<Atom>> generators = { {1s, 2s, Atom::H}, {1s, 4s, Atom::O} };

	for(auto& generator: generators)
		generator.start();

	return 0;
}
