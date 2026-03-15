#pragma once

#include "MemoryPool.h"
#include "OrderBook.h"
#include "Types.h"
#include "RingBuffer.h"
#include <vector>

namespace exchange {

struct ExecutionReport;

class MatchingEngine {
public:
  // Initialize with a given max capacity for orders and ring buffer
  explicit MatchingEngine(size_t max_orders = 1000000);

  // Process a new limit order
  void add_limit_order(OrderId id, Side side, Price price, Quantity visible_qty, Quantity total_qty = 0);

  // Process a market order (matches against best price until filled or book
  // empty)
  void add_market_order(OrderId id, Side side, Quantity quantity);

  // Cancel an existing order by ID
  void cancel_order(OrderId id);

  // For GUI rendering
  const OrderBook &get_order_book() const { return order_book_; }

  // Access to the Market Data Feed queue
  RingBuffer<ExecutionReport>& get_market_data_feed() { return market_feed_; }

private:
  OrderBook order_book_;
  MemoryPool<Order> order_pool_;
  RingBuffer<ExecutionReport> market_feed_;

  // Fast lookup for cancellations using direct array indexing O(1)
  std::vector<Order *> order_map_;

  // Helper to remove order from memory and map
  void cleanup_order(Order *order);
};

} // namespace exchange
