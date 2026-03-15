#include "OrderBook.h"
#include <algorithm>

namespace exchange {

bool OrderBook::add_order(Order *order) {
  if (!order || order->quantity == 0 || order->price >= MAX_PRICE)
    return false;

  if (order->side == Side::Buy) {
    bids_[order->price].add_order(order);
    update_max_bid(order->price);
  } else {
    asks_[order->price].add_order(order);
    update_min_ask(order->price);
  }
  return true;
}

void OrderBook::remove_order(Order *order) {
  if (!order || order->price >= MAX_PRICE)
    return;

  if (order->side == Side::Buy) {
    bids_[order->price].remove_order(order);
    if (order->price == max_bid_ && bids_[order->price].is_empty()) {
      while(max_bid_ > 0 && bids_[max_bid_].is_empty()) max_bid_--;
    }
  } else {
    asks_[order->price].remove_order(order);
    if (order->price == min_ask_ && asks_[order->price].is_empty()) {
      while(min_ask_ < MAX_PRICE - 1 && asks_[min_ask_].is_empty()) min_ask_++;
    }
  }
}

Order *OrderBook::get_best_bid() {
  if (max_bid_ == 0 && bids_[0].is_empty()) return nullptr;
  return bids_[max_bid_].head;
}

Order *OrderBook::get_best_ask() {
  if (min_ask_ >= MAX_PRICE) return nullptr;
  return asks_[min_ask_].head;
}

std::vector<std::pair<Price, const PriceLevel*>> OrderBook::get_top_bids(int count) const {
  std::vector<std::pair<Price, const PriceLevel*>> result;
  Price p = max_bid_;
  while(p > 0 && result.size() < static_cast<size_t>(count)) {
    if (!bids_[p].is_empty()) {
      result.push_back({p, &bids_[p]});
    }
    p--;
  }
  if (p == 0 && !bids_[0].is_empty() && result.size() < static_cast<size_t>(count)) {
    result.push_back({0, &bids_[0]});
  }
  return result;
}

std::vector<std::pair<Price, const PriceLevel*>> OrderBook::get_top_asks(int count) const {
  std::vector<std::pair<Price, const PriceLevel*>> result;
  Price p = min_ask_;
  while(p < MAX_PRICE && result.size() < static_cast<size_t>(count)) {
    if (!asks_[p].is_empty()) {
      result.push_back({p, &asks_[p]});
    }
    p++;
  }
  return result;
}

Quantity OrderBook::match_taker_order(Order *taker_order, std::vector<Order*>& filled_makers, RingBuffer<ExecutionReport>* market_feed) {
  if (!taker_order || taker_order->quantity == 0)
    return 0;

  Quantity initial_qty = taker_order->quantity;

  if (taker_order->side == Side::Buy) {
    while (min_ask_ < MAX_PRICE && taker_order->quantity > 0) {
      if (taker_order->price < min_ask_) break;

      PriceLevel &level = asks_[min_ask_];
      Order *maker_order = level.head;

      while (maker_order && taker_order->quantity > 0) {
        Quantity trade_qty = std::min(taker_order->quantity, maker_order->quantity);
        taker_order->quantity -= trade_qty;
        maker_order->quantity -= trade_qty;
        maker_order->total_remaining_qty -= trade_qty;
        level.total_volume -= trade_qty;

        Order *next_maker = maker_order->next;
        if (maker_order->quantity == 0) {
          level.remove_order(maker_order);
          // Iceberg Refill Logic
          if (maker_order->total_remaining_qty > 0) {
            maker_order->quantity = std::min(maker_order->display_qty, maker_order->total_remaining_qty); 
            level.add_order(maker_order); // goes to back of line
            // Push an execution report for the iceberg tranche that was filled
            if (market_feed) {
                market_feed->push({maker_order->id, maker_order->price, maker_order->display_qty, maker_order->side});
            }
          } else {
            filled_makers.push_back(maker_order);
          }
        }
        maker_order = next_maker;
      }

      if (level.is_empty()) {
        while(min_ask_ < MAX_PRICE - 1 && asks_[min_ask_].is_empty()) min_ask_++;
        if (asks_[min_ask_].is_empty()) min_ask_++; // reached MAX_PRICE
      } else {
        break;
      }
    }
  } else {
    while (taker_order->quantity > 0) {
      if (taker_order->price > max_bid_) break;
      if (max_bid_ == 0 && bids_[0].is_empty()) break;

      PriceLevel &level = bids_[max_bid_];
      Order *maker_order = level.head;

      while (maker_order && taker_order->quantity > 0) {
        Quantity trade_qty = std::min(taker_order->quantity, maker_order->quantity);
        taker_order->quantity -= trade_qty;
        maker_order->quantity -= trade_qty;
        maker_order->total_remaining_qty -= trade_qty;
        level.total_volume -= trade_qty;

        Order *next_maker = maker_order->next;
        if (maker_order->quantity == 0) {
          level.remove_order(maker_order);
          // Iceberg Refill Logic
          if (maker_order->total_remaining_qty > 0) {
             maker_order->quantity = std::min(maker_order->display_qty, maker_order->total_remaining_qty); 
             level.add_order(maker_order); // goes to back of line
             // Push an execution report for the iceberg tranche that was filled
             if (market_feed) {
                 market_feed->push({maker_order->id, maker_order->price, maker_order->display_qty, maker_order->side});
             }
          } else {
            filled_makers.push_back(maker_order);
          }
        }
        maker_order = next_maker;
      }

      if (level.is_empty()) {
        if (max_bid_ == 0) break;
        while(max_bid_ > 0 && bids_[max_bid_].is_empty()) max_bid_--;
      } else {
        break;
      }
    }
  }

  return initial_qty - taker_order->quantity;
}

} // namespace exchange
