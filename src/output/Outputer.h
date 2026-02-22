#pragma once

#include <iostream>
#include <wal/WALShared.h>
#include <TradeEvent/TradeEvent.h>
#include <stats/StatsTransfer.h>
#include <format>
#include <print>
#include <chrono>
#include <cstdio>
#include <memory>
#include <time/Time.h>
namespace lob {

	enum class OutputOptions {
		TO_CONSOLE,
		TO_FILE
	};
	
	static const char* FILE_EVENTS_NAME_OUTPUT = "EVENTS.txt";
	static const char* FILE_SNAPSHOTS_NAME_OUTPUT = "SNAPSHOTS.txt";

	static constexpr size_t FILE_EVENTS_BUFFER_SIZE = 1024 * 1024;
	static constexpr size_t FILE_SNAPSHOTS_BUFFER_SIZE = 1024;

	struct FileDeleter {
		void operator()(FILE* f) const { if (f) std::fclose(f); }
	};

	template<OutputOptions events_output_option, OutputOptions snapshots_outout_option>
	class ConsoleOutput {
	private:
		using unique_file = std::unique_ptr<FILE, FileDeleter>;
		unique_file events_file;
		unique_file snapshots_file;

		WALQueue& file_queue;
		WALSnapshotStatsQueue& snapshots_queue;

		FastTime fast_time;

		void printTradeEvent(const TradeEvent& e) noexcept {
			using namespace std::chrono;
			nanoseconds ns(fast_time.tsc_to_absolute_ns(e.timestamp));
			auto tp = system_clock::time_point(duration_cast<system_clock::duration>(ns));

			if constexpr (events_output_option == OutputOptions::TO_FILE) {
				std::print(events_file.get(),
					"[{:%H:%M:%S}] ID:{:<8} | {:<4} | P:{:>10} | Q:{:>6} | T:{} M:{}\n",
					tp,
					e.trade_id,
					(e.taker_side == Side::Buy ? "BUY" : "SELL"),
					e.price,
					e.quantity,
					e.taker_order_id,
					e.maker_order_id
				);
			}
			else {
				std::print(
					"[{:%H:%M:%S}] ID:{:<8} | {:<4} | P:{:>10} | Q:{:>6} | T:{} M:{}\n",
					tp,
					e.trade_id,
					(e.taker_side == Side::Buy ? "BUY" : "SELL"),
					e.price,
					e.quantity,
					e.taker_order_id,
					e.maker_order_id
				);
			}
		}

		void printSnapshot(StatsTransfer& transfer) noexcept {
			if constexpr (snapshots_outout_option == OutputOptions::TO_FILE) {
				transfer.common_stats.print_report(fast_time, snapshots_file.get());
				transfer.percentile_stats.print_report(fast_time, snapshots_file.get());
			}
			else {
				transfer.common_stats.print_report(fast_time);
				transfer.percentile_stats.print_report(fast_time);
			}
		}

	public:
		ConsoleOutput(WALQueue& file_queue_, WALSnapshotStatsQueue& snapshots_queue_) : 
			file_queue(file_queue_), snapshots_queue(snapshots_queue_){
			if constexpr (events_output_option == OutputOptions::TO_FILE) {
				if constexpr (CLEAR_OUTPUT_FILES) {
					events_file.reset(std::fopen(FILE_EVENTS_NAME_OUTPUT, "wb"));
				}
				else {
					events_file.reset(std::fopen(FILE_EVENTS_NAME_OUTPUT, "ab"));
				}

				if (events_file) {
					static char events_buffer[FILE_EVENTS_BUFFER_SIZE];
					std::setvbuf(events_file.get(), events_buffer, _IOFBF, sizeof(events_buffer));
				}
			}
			if constexpr (snapshots_outout_option == OutputOptions::TO_FILE) {
				if constexpr (CLEAR_OUTPUT_FILES) {
					snapshots_file.reset(std::fopen(FILE_SNAPSHOTS_NAME_OUTPUT, "wb"));
				}
				else {
					snapshots_file.reset(std::fopen(FILE_SNAPSHOTS_NAME_OUTPUT, "ab"));
				}

				if (snapshots_file) {
					static char snapshots_buffer[FILE_EVENTS_BUFFER_SIZE];
					std::setvbuf(snapshots_file.get(), snapshots_buffer, _IOFBF, sizeof(snapshots_buffer));
				}
			}
		}
		void process(std::stop_token stoken) {

			TradeEvent event;
			StatsTransfer stats_transfer;
			while (!stoken.stop_requested()) {

				if (file_queue.try_pop_object(event)) {
					printTradeEvent(event);
				}
				if (snapshots_queue.try_pop_object(stats_transfer)) {
					printSnapshot(stats_transfer);
				}
				smart_pause();

			}


		}


	};
}