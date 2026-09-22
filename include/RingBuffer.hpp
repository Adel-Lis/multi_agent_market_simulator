//
// Adel Lis created RingBuffer on 22/09/2026.
//

#pragma once

#include <cassert>
#include <cstddef>
#include <vector>

namespace cda
{
    template <typename T>
    class RingBuffer
    {
    public:
        /// @param capacity the number of values retained. positive
        explicit RingBuffer(std::size_t capacity) : data_(capacity)
        {
            assert(capacity > 0 && "RingBuffer capacity must be positive");
        }

        /// Append a value, overwriting the oldest one if the buffer is full.
        void push(const T& value)
        {
            data_[head_] = value;
            head_ = (head_ + 1) % data_.size();
            if (size_ < data_.size()) ++size_;
        }

        /// The value pushed `lag` pushes ago: ago(0) is the most recent. Requires lag < size().
        const T& ago(std::size_t lag) const
        {
            assert(lag < size_ && "RingBuffer::ago lag out of range");
            const std::size_t cap = data_.size();
            return data_[(head_ + cap - 1 - lag) % cap];
        }

        std::size_t size() const { return size_; }
        std::size_t capacity() const { return data_.size(); }
        bool empty() const { return size_ == 0; }
        bool full() const { return size_ == data_.size(); }

    private:
        std::vector<T> data_;
        std::size_t head_ = 0;
        std::size_t size_ = 0;
    };
} // namespace cda
