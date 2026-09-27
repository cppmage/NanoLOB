#pragma once
#include <cstdint>
#include <cstddef>

namespace lob
{
    /*
    * ID structure
    * [EPOCH][POSITION IN POOL]
    */
    inline constexpr std::size_t id_size = 64;
    inline constexpr std::size_t order_position_bits = 21;
    inline constexpr std::size_t order_epoch_bits = id_size - order_position_bits;

    using pool_size_t = uint32_t;

    inline constexpr std::size_t cache_line_size = 64;

    inline constexpr std::size_t orders_per_lob = (1ULL << order_position_bits) - 1;

} // namespace lob
