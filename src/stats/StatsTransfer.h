#pragma once
#include "stats/CommonStats.h"
#include "stats/PercentileStats.h"

namespace lob {

	class StatsTransfer {

	private:

	public:
		CommonStats common_stats;
		PercentileStats percentile_stats;

	};

}