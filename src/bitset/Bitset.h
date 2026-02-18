#pragma once
#include <array>
#include <new>

namespace lob {

	using bit_container = uint64_t;
	static constexpr size_t bit_container_size = sizeof(bit_container);
	static constexpr size_t bits_mult_layer1 = bit_container_size * 8;
	static constexpr size_t bits_mult_layer2 = bit_container_size * 8;
	static constexpr size_t BITSET_EMPTY_FLAG_VALUE = UINT64_MAX;

	template<size_t bits, size_t size0 = (bits + bits_mult_layer1 - 1) / bits_mult_layer1, size_t size1 = ((bits + bits_mult_layer1 - 1) / bits_mult_layer1 + bits_mult_layer2-1)/ bits_mult_layer2>
	class Bitset {
	private:
		alignas(std::hardware_destructive_interference_size) std::array<bit_container, size0> l1_mask;
		alignas(std::hardware_destructive_interference_size) std::array<bit_container, size1> l2_mask;
	public:

		Bitset() {
			l1_mask.fill(0);
			l2_mask.fill(0);
		}
		size_t firstNotZeroBit() const noexcept {
			for (size_t i = 0; i < size1; ++i) {
				if (l2_mask[i] != 0) {
					unsigned long l2_pos;
					_BitScanForward64(&l2_pos, l2_mask[i]);

					size_t l1_idx = (i * bits_mult_layer2) + l2_pos;
					unsigned long l1_pos;
					_BitScanForward64(&l1_pos, l1_mask[l1_idx]);

					return (l1_idx * bits_mult_layer1) + l1_pos;
				}
			}
			return UINT64_MAX;
		}
		size_t lastNotZeroBit() const noexcept {
			for (int i = (int)size0 - 1; i >= 0; i--) {
				if (l1_mask[i] != 0) {
					unsigned long pos = 0;
					_BitScanReverse64(&pos, l1_mask[i]); 
					return (i * bits_mult_layer1) + pos;
				}
			}
			return UINT64_MAX;
		}
		inline void set(size_t id) noexcept {
			size_t mass_i = id / bits_mult_layer1;
			int bit_offset = id & (bits_mult_layer1 -1);

			bit_container mask = 1ULL << bit_offset;

			l1_mask[mass_i] |= mask;
			l2_mask[mass_i / bits_mult_layer2] |= (1ULL << (mass_i & (bits_mult_layer2 - 1)));
		}
		void reset(size_t id) noexcept {
			size_t mass_i = id / bits_mult_layer1;
			int bit_offset = id & (bits_mult_layer1 - 1);

			bit_container mask = ~(1ULL << bit_offset);

			auto& value = l1_mask[mass_i];
			l1_mask[mass_i] &= mask;

			if (l1_mask[mass_i] == 0) {
				l2_mask[mass_i / bits_mult_layer2] &= ~(1ULL << (mass_i & (bits_mult_layer2 - 1)));
			}
		}

	};

}