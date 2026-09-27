#pragma once

#include <other/Other.hpp>
#include <time/Time.hpp>

namespace lob {

	class TicksTimer {
	private:
		uint64_t end;
	public:
		TicksTimer() {
			reset();
		}
		inline void reset() noexcept {
			end = get_ticks();
		}
		inline bool ensure() noexcept {
			return get_ticks() >= end;
		}
		inline void set(uint64_t ticks) noexcept {
			end = get_ticks() + ticks;
		}
	};

}