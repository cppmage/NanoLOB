#pragma once

#include <iostream>
#include <wal/WALShared.h>
#include <TradeEvent/TradeEvent.h>
#include <format>
#include <print>
#include <chrono>

namespace lob {
	class ConsoleOutput {
	private:
		WALQueue& file_queue;
		
		void printTradeEvent(const TradeEvent& e) {
			using namespace std::chrono;
			nanoseconds ns(e.timestamp);
			auto tp = system_clock::time_point(duration_cast<system_clock::duration>(ns));

			std::print(
				"[{:%H:%M:%S}.{:09}] ID:{:<8} | {:<4} | P:{:>10} | Q:{:>6} | T:{} M:{}\n",
				tp,                           
				e.timestamp % 1'000'000'000, 
				e.trade_id,
				(e.taker_side == Side::Buy ? "BUY" : "SELL"),
				e.price,
				e.quantity,
				e.taker_order_id,
				e.maker_order_id
			);
		}

	public:
		ConsoleOutput(WALQueue& file_queue_) : file_queue(file_queue_){

		}
		void process(std::stop_token stoken) {

			TradeEvent event;
			while (!stoken.stop_requested()) {

				if (file_queue.try_pop_object(event)) {
					printTradeEvent(event);
				}
				CPU_PAUSE();

			}


		}


	};
}