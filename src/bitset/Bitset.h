#pragma once
#include <array>
#include <new>
#include <cstddef>
#include <stdint.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace lob {

	

	using bit_container = uint64_t;
	static constexpr size_t bit_container_size = sizeof(bit_container);
	static constexpr size_t bit_container_len_bits = bit_container_size * 8 - 1;
	static constexpr size_t bits_per_layer = bit_container_size * 8;
	static constexpr size_t BITSET_EMPTY_FLAG_VALUE = UINT64_MAX;


	inline bool BitScanForward(unsigned long* index, bit_container mask) {
		if (mask == 0) return false;
	#ifdef _MSC_VER
		return _BitScanForward64(index, mask);
	#else
		* index = (unsigned long)__builtin_ctzll(mask);
		return true;
	#endif
	}

	inline bool BitScanReverse(unsigned long* index, bit_container mask) {
		if (mask == 0) return false;
	#ifdef _MSC_VER
		return _BitScanReverse64(index, mask);
	#else
		* index = bit_container_len_bits - __builtin_clzll(mask);
		return true;
	#endif
	}

	template<size_t bits>
	class Bitset {
	private:
		static constexpr size_t size1 = (bits + bits_per_layer - 1) / bits_per_layer;
		static constexpr size_t size2 = (size1 + bits_per_layer - 1) / bits_per_layer;
		static constexpr size_t size3 = (size1 + bits_per_layer - 1) / bits_per_layer;

		alignas(std::hardware_destructive_interference_size) std::array<bit_container, size1> l1_mask;
		alignas(std::hardware_destructive_interference_size) std::array<bit_container, size2> l2_mask;
	public:

		Bitset() {
			l1_mask.fill(0);
			l2_mask.fill(0);
		}
		size_t firstNotZeroBit() const noexcept {
			for (size_t i = 0; i < size2; ++i) {
				if (l2_mask[i] != 0) {
					unsigned long l2_pos;
					BitScanForward(&l2_pos, l2_mask[i]);

					size_t l1_idx = (i * bits_per_layer) + l2_pos;
					unsigned long l1_pos;
					BitScanForward(&l1_pos, l1_mask[l1_idx]);

					return (l1_idx * bits_per_layer) + l1_pos;
				}
			}
			return UINT64_MAX;
		}
		size_t lastNotZeroBit() const noexcept {
			for (size_t i = size2;i -- >0;) {
				if (l2_mask[i] != 0) {
					unsigned long l2_pos;
					BitScanReverse(&l2_pos, l2_mask[i]);

					size_t l1_idx = (i * bits_per_layer) + l2_pos;
					unsigned long l1_pos;
					BitScanReverse(&l1_pos, l1_mask[l1_idx]);

					return (l1_idx * bits_per_layer) + l1_pos;
				}
			}

			return UINT64_MAX;
		}
		inline void set(size_t id) noexcept {
			size_t mass_i = id / bits_per_layer;
			int bit_offset = id & (bits_per_layer -1);

			bit_container mask = 1ULL << bit_offset;

			l1_mask[mass_i] |= mask;
			l2_mask[mass_i / bits_per_layer] |= (1ULL << (mass_i & (bits_per_layer - 1)));
		}
		void reset(size_t id) noexcept {
			size_t mass_i = id / bits_per_layer;
			int bit_offset = id & (bits_per_layer - 1);

			bit_container mask = ~(1ULL << bit_offset);

			auto& value = l1_mask[mass_i];
			l1_mask[mass_i] &= mask;

			if (l1_mask[mass_i] == 0) {
				l2_mask[mass_i / bits_per_layer] &= ~(1ULL << (mass_i & (bits_per_layer - 1)));
			}
		}

	};

}