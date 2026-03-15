#include "MatchingEngine.h"

namespace exchange {

MatchingEngine::MatchingEngine(size_t max_orders) : order_pool_(max_orders), market_feed_(max_orders), order_map_(max_orders * 2, nullptr) {
}

void MatchingEngine::cleanup_order(Order *order) {
  if (!order)
    return;
  if (order->id < order_map_.size()) {
    order_map_[order->id] = nullptr;
  }
  order_pool_.deallocate(order);
}

void MatchingEngine::add_limit_order(OrderId id, Side side, Price price, Quantity visible_qty, Quantity total_qty) {
  // We expand the mapping array if ID goes out of bounds. In a true HFT context, IDs are bounded.
  if (id >= order_map_.size()) {
    order_map_.resize(id * 2, nullptr);
  }

  if (visible_qty == 0 || order_map_[id] != nullptr)
    return; // Reject 0 qty or duplicate

  Order taker_order(id, side, price, visible_qty, total_qty);

  std::vector<Order*> filled_makers;
  Quantity filled_qty = order_book_.match_taker_order(&taker_order, filled_makers, &market_feed_);

  // If a trade happened, we push an Execution Report for the taker
  if (filled_qty > 0) {
      market_feed_.push({taker_order.id, taker_order.price, filled_qty, taker_order.side});
  }

  // O(1) cleanup per filled order instead of an O(N) sweep
  for (Order *filled_order : filled_makers) {
    // Send Execution Report for the Maker that was fully filled
    market_feed_.push({filled_order->id, filled_order->price, filled_order->display_qty, filled_order->side});
    cleanup_order(filled_order);
  }

  if (taker_order.quantity > 0) {
    Order *new_order =
        order_pool_.allocate(taker_order.id, taker_order.side,
                             taker_order.price, taker_order.quantity, taker_order.total_remaining_qty);
    if (new_order) {
      order_book_.add_order(new_order);
      order_map_[new_order->id] = new_order;
    }
  }
}

void MatchingEngine::add_market_order(OrderId id, Side side, Quantity quantity) {
  // A market order is just a limit order with an extreme price
  if (id >= order_map_.size()) {
    order_map_.resize(id * 2, nullptr);
  }

  if (quantity == 0 || order_map_[id] != nullptr)
    return;

  Price extreme_price = (side == Side::Buy) ? MAX_PRICE - 1 : 0;

  Order taker_order(id, side, extreme_price, quantity);
  
  std::vector<Order*> filled_makers;
  Quantity filled_qty = order_book_.match_taker_order(&taker_order, filled_makers, &market_feed_);

  if (filled_qty > 0) {
      market_feed_.push({taker_order.id, taker_order.price, filled_qty, taker_order.side});
  }

  for (Order *filled_order : filled_makers) {
    market_feed_.push({filled_order->id, filled_order->price, filled_order->display_qty, filled_order->side});
    cleanup_order(filled_order);
  }
}

void MatchingEngine::cancel_order(OrderId id) {
  if (id >= order_map_.size()) return;
  
  Order *order = order_map_[id];
  if (order) {
    order_book_.remove_order(order);
    
    // We can also send a cancel report
    market_feed_.push({order->id, order->price, 0, order->side});
    cleanup_order(order);
  }
}

} // namespace exchange
