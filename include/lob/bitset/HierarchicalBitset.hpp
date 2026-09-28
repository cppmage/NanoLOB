#pragma once
#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <lob/parameters/parameters.hpp>

namespace lob {

	using bit_container = uint64_t;
	static constexpr size_t bits_per_layer = sizeof(bit_container) * 8;
	static constexpr size_t BITSET_EMPTY_FLAG_VALUE = UINT64_MAX;

	namespace bitset_detail {

		constexpr size_t words_for(size_t n) noexcept {
			return (n + bits_per_layer - 1) / bits_per_layer;
		}

		constexpr size_t count_levels(size_t bits, size_t top_words) noexcept {
			size_t words = words_for(bits);
			size_t levels = 1;
			while (words > top_words) {
				words = words_for(words);
				++levels;
			}
			return levels;
		}

		template<size_t levels>
		constexpr std::array<size_t, levels + 1> make_offsets(size_t bits) noexcept {
			std::array<size_t, levels> words{};
			size_t n = words_for(bits);
			for (size_t level = levels; level-- > 0;) {
				words[level] = n;
				n = words_for(n);
			}

			std::array<size_t, levels + 1> offsets{};
			for (size_t level = 0; level < levels; ++level) {
				offsets[level + 1] = offsets[level] + words[level];
			}
			return offsets;
		}

		inline size_t lowest(bit_container mask) noexcept {
			return static_cast<size_t>(std::countr_zero(mask));
		}

		inline size_t highest(bit_container mask) noexcept {
			return bits_per_layer - 1 - static_cast<size_t>(std::countl_zero(mask));
		}
	}

	template<size_t bits, size_t top_words>
	class HierarchicalBitset {
	private:
		static_assert(bits > 0, "bitset must hold at least one bit");
		static_assert(top_words > 0, "top level must hold at least one word");

		static constexpr size_t levels = bitset_detail::count_levels(bits, top_words);
		static constexpr std::array<size_t, levels + 1> offsets = bitset_detail::make_offsets<levels>(bits);
		static constexpr size_t top_level_words = offsets[1] - offsets[0];
		static constexpr size_t total_words = offsets[levels];

		alignas(cache_line_size) std::array<bit_container, total_words> data;

	public:
		HierarchicalBitset() {
			data.fill(0);
		}

		size_t firstNotZeroBit() const noexcept {
			for (size_t word = 0; word < top_level_words; ++word) {
				bit_container mask = data[word];
				if (mask == 0) {
					continue;
				}

				size_t index = word * bits_per_layer + bitset_detail::lowest(mask);
				for (size_t level = 1; level < levels; ++level) {
					index = index * bits_per_layer + bitset_detail::lowest(data[offsets[level] + index]);
				}
				return index;
			}
			return BITSET_EMPTY_FLAG_VALUE;
		}

		size_t lastNotZeroBit() const noexcept {
			for (size_t word = top_level_words; word-- > 0;) {
				bit_container mask = data[word];
				if (mask == 0) {
					continue;
				}

				size_t index = word * bits_per_layer + bitset_detail::highest(mask);
				for (size_t level = 1; level < levels; ++level) {
					index = index * bits_per_layer + bitset_detail::highest(data[offsets[level] + index]);
				}
				return index;
			}
			return BITSET_EMPTY_FLAG_VALUE;
		}

		void set(size_t id) noexcept {
			assert(id < bits);
			size_t index = id;
			for (size_t level = levels; level-- > 0;) {
				bit_container& word = data[offsets[level] + index / bits_per_layer];
				bool was_empty = word == 0;
				word |= bit_container{1} << (index % bits_per_layer);
				if (!was_empty) {
					return;
				}
				index /= bits_per_layer;
			}
		}

		void reset(size_t id) noexcept {
			assert(id < bits);
			size_t index = id;
			for (size_t level = levels; level-- > 0;) {
				bit_container& word = data[offsets[level] + index / bits_per_layer];
				word &= ~(bit_container{1} << (index % bits_per_layer));
				if (word != 0) {
					return;
				}
				index /= bits_per_layer;
			}
		}
	};

}
