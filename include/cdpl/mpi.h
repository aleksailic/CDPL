/*
	Concurrent and Distributed Programming Library for C++
	Copyright (C) 2019 Aleksa Ilic <aleksa.d.ilic@gmail.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef CDPL_MPI_H_
#define CDPL_MPI_H_

namespace cdpl {
	namespace mpi {
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
}

#endif
