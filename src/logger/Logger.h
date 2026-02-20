#pragma once

#include "lockfree/SPSCQueue.h"
#include "TradeEvent/TradeEvent.h"
#include "wal/WALShared.h"
#include <thread>
#include "stats/StatsTransfer.h"
#include "time/Time.h"

namespace lob {

	class Logger {
	private:

		StatsTransfer transfer;
		FastTime time;

		TradeEventsQueue& trade_queue;
		WALQueue& shared_file;
		WALSnapshotStatsQueue& shared_snapshots_file;
		CommonStats& common_stats;
		PercentileStats& percentile_stats;

	public:
		Logger(TradeEventsQueue& trade_queue_, WALQueue& shared_file_,
			WALSnapshotStatsQueue& shared_snapshots_file_) : trade_queue(trade_queue_), shared_file(shared_file_),
			common_stats(transfer.common_stats), percentile_stats(transfer.percentile_stats),
			shared_snapshots_file(shared_snapshots_file_){

		}
        void process(std::stop_token stoken) {
            static constexpr size_t BATCH_SIZE = 128; // Увеличь до 128
            std::array<TradeEvent, BATCH_SIZE> buffer;
            size_t n = 0;

            while (!stoken.stop_requested()) {
                // 1. Жадное выгребание (освобождаем стакан ПЕРВЫМ делом)
                while (n < BATCH_SIZE) {
                    if (auto* p = trade_queue.prepare_pop()) {
                        buffer[n++] = *p;
                        trade_queue.commit_pop();
                    }
                    else break;
                }

                if (n > 0) {
                    uint64_t t_log = get_ticks();

                    // 2. Статистика (L1-стек, очень быстро)
                    for (size_t i = 0; i < n; ++i) {
                        common_stats.update(buffer[i], t_log);
                        percentile_stats.update(buffer[i].dt_match);
                    }

                    // 3. Пакетная запись (Самое важное!)
                    // Вместо цикла с try_push_object сделай один memcpy в mmap
                    if (!shared_file.try_push_batch(buffer.data(), n)) {
                        while (!shared_file.try_push_batch(buffer.data(), n)) {
                            CPU_PAUSE();
                            if (stoken.stop_requested()) break;
                        }
                    }
                    n = 0;
                }
                else {
                    CPU_PAUSE();
                }
            }
        }
		~Logger() {
			common_stats.print_report(time);
			percentile_stats.print_report(time);
		}
	};

}