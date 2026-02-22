#pragma once

#include "lockfree/SPSCQueue.h"
#include "TradeEvent/TradeEvent.h"
#include "wal/WALShared.h"
#include <thread>
#include "stats/StatsTransfer.h"
#include "time/Time.h"
#include "time/TicksTimer.h"

namespace lob {

    static const uint64_t ns_per_snapshot = NANOSECONDS_PER_STATS_SNAPSHOT;

	class Logger {
	private:


		StatsTransfer transfer;
		FastTime time;
        TicksTimer timer;

		TradeEventsQueue& trade_queue;
		WALQueue& shared_file;
		WALSnapshotStatsQueue& shared_snapshots_file;
		CommonStats& common_stats;
		PercentileStats& percentile_stats;

        uint64_t ticks_for_snapshot_timer;

	public:
		Logger(TradeEventsQueue& trade_queue_, WALQueue& shared_file_,
			WALSnapshotStatsQueue& shared_snapshots_file_) : trade_queue(trade_queue_), shared_file(shared_file_),
			common_stats(transfer.common_stats), percentile_stats(transfer.percentile_stats),
			shared_snapshots_file(shared_snapshots_file_)
        {
            ticks_for_snapshot_timer = time.ns_to_ticks(ns_per_snapshot);
            timer.set(ticks_for_snapshot_timer);
		}
        void process(std::stop_token stoken) {
            static constexpr size_t BATCH_SIZE = 32; 
            std::array<TradeEvent, BATCH_SIZE> buffer;
            size_t n = 0;

            while (!stoken.stop_requested()) {
                while (n < BATCH_SIZE) {
                    if (auto* p = trade_queue.prepare_pop()) {
                        buffer[n++] = *p;
                        trade_queue.commit_pop();
                    }
                    else break;
                }

                if (n > 0) {
                    uint64_t t_log = get_ticks();

                    for (size_t i = 0; i < n; ++i) {
                        common_stats.update(buffer[i], t_log);
                        percentile_stats.update(buffer[i].dt_match);
                    }

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

                if (timer.ensure()) {
                    if (shared_snapshots_file.try_push_object(transfer)) [[likely]] {
                        if constexpr (CLEAR_LOGGER_STATS_AFTER_STAPSHOT) {
                            transfer.reset();
                        }
                        timer.set(ticks_for_snapshot_timer);
                    }
                }
            

            }

        }
		~Logger() {
			common_stats.print_report(time);
			percentile_stats.print_report(time);
		}
	};

}