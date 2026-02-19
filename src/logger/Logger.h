#pragma once

#include "lockfree/SPSCQueue.h"
#include "TradeEvent/TradeEvent.h"
#include "wal/WALShared.h"
#include <thread>


namespace lob {

	class Logger {
	private:
		TradeEventsQueue& trade_queue;
		WALQueue& shared_file;

		 

	public:
		Logger(TradeEventsQueue& trade_queue_, WALQueue& shared_file_) : trade_queue(trade_queue_), shared_file(shared_file_){

		}
		void process(std::stop_token stoken) {
			TradeEvent* ptr=nullptr;

			static constexpr size_t BATCH_SIZE = 32;
			std::array<TradeEvent, BATCH_SIZE> batch_buffer;

			size_t collected = 0;

			while (!stoken.stop_requested()) {
				while (collected < BATCH_SIZE) {
					TradeEvent* ptr = trade_queue.prepare_pop();
					if (!ptr) break;

					batch_buffer[collected] = *ptr;
					trade_queue.commit_pop();
					collected++;


				}

				if (collected > 0) {
					for (size_t i = 0; i < collected; ++i) {
						while (!shared_file.try_push_object(batch_buffer[i])) {
							CPU_PAUSE();
							if (stoken.stop_requested())return;
						}
					}
					collected = 0; 
				}
				else {
					CPU_PAUSE();
				}

			}

		}
	};

}