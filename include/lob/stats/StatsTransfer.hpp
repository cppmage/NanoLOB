#pragma once
#include "stats/CommonStats.hpp"
#include "stats/PercentileStats.hpp"

namespace lob {

	class StatsTransfer {

	private:

	public:
		CommonStats common_stats;
		PercentileStats percentile_stats;
		void reset() noexcept {
			common_stats.reset();
			percentile_stats.reset();
		}
	};

}