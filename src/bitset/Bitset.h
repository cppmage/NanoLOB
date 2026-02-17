#pragma once
#include <array>
#include <new>

namespace lob {

	using bit_container = uint64_t;
	static constexpr size_t bit_container_size = sizeof(bit_container);
	static constexpr size_t bits_mult = bit_container_size * 8;
	static constexpr size_t BITSET_EMPTY_FLAG_VALUE = UINT64_MAX;

	template<size_t bits, size_t size = (bits + bits_mult - 1) / bits_mult>
	class Bitset {
	private:
		alignas(std::hardware_destructive_interference_size) std::array<bit_container, size> mass;
	public:

		Bitset() {
			std::fill(mass.begin(), mass.end(), 0);
		}
		size_t firstNotZeroBit() noexcept {
			for (size_t i = 0; i < size; i++) {
				if (mass[i] != 0) {
					unsigned long pos=0;
					_BitScanForward64(&pos, mass[i]);
					return (i * bits_mult) + pos;
				}
			}
			return UINT64_MAX;
		}
		size_t lastNotZeroBit() noexcept {
			for (int i = (int)size - 1; i >= 0; i--) {
				if (mass[i] != 0) {
					unsigned long pos = 0;
					_BitScanReverse64(&pos, mass[i]); 
					return (i * bits_mult) + pos;
				}
			}
			return UINT64_MAX;
		}
		void set(size_t id) noexcept {
			size_t mass_i = id / bits_mult;
			int bit_offset = id & (bits_mult -1);

			bit_container mask = 1ULL << bit_offset;

			mass[mass_i] |= mask;
		}
		void reset(size_t id) noexcept {
			size_t mass_i = id / bits_mult;
			int bit_offset = id & (bits_mult - 1);

			bit_container mask = ~(1ULL << bit_offset);

			auto& value = mass[mass_i];
			mass[mass_i] &= mask;
		}

	};

}