#pragma once
#include <array>
#include <lob/parameters/parameters.hpp>

namespace lob
{
    template<typename T, pool_size_t size>
    class FIFO_FreeList{
        private:
        std::array<T, size> arr;
        pool_size_t tail, head;

        public:
        FIFO_FreeList() : tail(0), head(0) {}

        bool empty() const noexcept {
            return tail==head;
        }

        T pop() noexcept {
            T res = arr[head];
            head = (head + 1) % size;
            return res;
        }

        template<typename U>
        void push(U&& u) noexcept {
            arr[tail] = std::forward<U>(u);
            tail = (tail+1)%size;
        }
    };
} // namespace lob
