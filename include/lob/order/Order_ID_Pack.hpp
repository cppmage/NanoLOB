#pragma once
#include <bit>
#include <cstdint>
#include <tuple>
#include <lob/parameters/parameters.hpp>

namespace lob
{
    struct Order_ID_Pack {
        uint64_t position : order_position_bits;
        uint64_t epoch : order_epoch_bits;
        constexpr Order_ID_Pack() : position(0), epoch(0) {}

        explicit constexpr Order_ID_Pack(uint64_t full_value) {
            position = full_value & ((1ULL << order_position_bits) - 1);
            epoch = (full_value >> order_position_bits) & ((1ULL << order_epoch_bits) - 1);
        }
        constexpr Order_ID_Pack(uint64_t pos, uint64_t ep) : position(pos), epoch(ep) {}

        explicit constexpr operator uint64_t() const noexcept {
            return std::bit_cast<uint64_t>(*this);
        }

        bool operator==(const Order_ID_Pack& other) const {
            return position == other.position && epoch == other.epoch;
        }

        bool operator<(const Order_ID_Pack& other) const {
            return std::tie(epoch, position) < std::tie(other.epoch, other.position);
        }

        bool operator>(const Order_ID_Pack& other) const {
            return std::tie(epoch, position) > std::tie(other.epoch, other.position);
        }
    };

    static_assert(sizeof(Order_ID_Pack) == sizeof(uint64_t), "Order Id struct must be 64 bits");

    static_assert(std::bit_cast<uint64_t>(Order_ID_Pack(1, 1)) ==
                      ((1ULL << order_position_bits) | 1ULL),
                  "bitfield layout is not [EPOCH][POSITION IN POOL]");
    static_assert(std::bit_cast<uint64_t>(Order_ID_Pack(orders_per_lob, 0)) == orders_per_lob,
                  "position must occupy the low bits");
} // namespace lob
