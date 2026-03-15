#pragma once

#include <atomic>
#include <vector>
#include <cassert>

namespace exchange {

/**
 * A fast, Lock-Free Single-Producer Single-Consumer (SPSC) Ring Buffer.
 * Used to pipe Execution Reports out of the Matching Engine without stalling the critical path.
 */
template <typename T> class RingBuffer {
public:
  explicit RingBuffer(size_t capacity) : capacity_(capacity), head_(0), tail_(0) {
    // Capacity should ideally be a power of 2 for bitwise masking, but we'll use modulo for simplicity if not enforced
    buffer_.resize(capacity_);
  }

  // Called by Producer (Matching Engine)
  bool push(const T& item) {
    const size_t current_head = head_.load(std::memory_order_relaxed);
    const size_t next_head = (current_head + 1) % capacity_;

    // If next_head hits tail, buffer is full
    if (next_head == tail_.load(std::memory_order_acquire)) {
      return false; // Queue full
    }

    buffer_[current_head] = item;
    
    // Release ensures the write to buffer is visible to consumer before head updates
    head_.store(next_head, std::memory_order_release);
    return true;
  }

  // Called by Consumer (Market Data Protocol Thread)
  bool pop(T& item) {
    const size_t current_tail = tail_.load(std::memory_order_relaxed);

    // If tail == head, buffer is empty
    if (current_tail == head_.load(std::memory_order_acquire)) {
      return false; 
    }

    item = buffer_[current_tail];

    // Release ensures the read is complete before we update tail
    tail_.store((current_tail + 1) % capacity_, std::memory_order_release);
    return true;
  }

private:
  size_t capacity_;
  std::vector<T> buffer_;
  alignas(64) std::atomic<size_t> head_; // Producer writes
  alignas(64) std::atomic<size_t> tail_; // Consumer reads
};

} // namespace exchange
