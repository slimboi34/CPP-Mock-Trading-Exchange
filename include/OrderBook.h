#pragma once

#include "Types.h"
#include "RingBuffer.h"
#include <vector>

namespace exchange {

struct ExecutionReport;

constexpr Price MAX_PRICE = 200000;

struct PriceLevel {
  Order *head = nullptr;
  Order *tail = nullptr;
  Quantity total_volume = 0;

  void add_order(Order *order) {
    order->prev = tail;
    order->next = nullptr;
    if (tail) {
      tail->next = order;
    } else {
      head = order;
    }
    tail = order;
    total_volume += order->quantity;
  }

  void remove_order(Order *order) {
    if (order->prev) {
      order->prev->next = order->next;
    } else {
      head = order->next;
    }

    if (order->next) {
      order->next->prev = order->prev;
    } else {
      tail = order->prev;
    }

    total_volume -= order->quantity;
    order->prev = nullptr;
    order->next = nullptr;
  }

  bool is_empty() const { return head == nullptr; }
};

class OrderBook {
public:
  OrderBook() : bids_(MAX_PRICE), asks_(MAX_PRICE) {}

  bool add_order(Order *order);
  void remove_order(Order *order);

  Order *get_best_bid();
  Order *get_best_ask();

  bool has_bids() const { return max_bid_ > 0 || !bids_[0].is_empty(); }
  bool has_asks() const { return min_ask_ < MAX_PRICE; }

  // Try to match a taking order against the book.
  // Fills the filled_makers vector with fully matched orders for backend cleanup.
  // Optionally passes the market_feed to push out partial iceberg tranche executions.
  Quantity match_taker_order(Order *taker_order, std::vector<Order*>& filled_makers, RingBuffer<ExecutionReport>* market_feed = nullptr);

  // For GUI rendering
  std::vector<std::pair<Price, const PriceLevel*>> get_top_bids(int count) const;
  std::vector<std::pair<Price, const PriceLevel*>> get_top_asks(int count) const;

private:
  std::vector<PriceLevel> bids_;
  std::vector<PriceLevel> asks_;
  Price max_bid_ = 0;
  Price min_ask_ = MAX_PRICE;

  void inline update_max_bid(Price price) {
    if (price > max_bid_) max_bid_ = price;
  }
  void inline update_min_ask(Price price) {
    if (price < min_ask_) min_ask_ = price;
  }
};

} // namespace exchange
