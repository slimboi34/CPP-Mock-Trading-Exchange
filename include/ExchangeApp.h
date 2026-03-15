#pragma once

#include "MatchingEngine.h"
#include <vector>
#include <deque>

namespace exchange {

class ExchangeApp {
public:
  ExchangeApp();

  // Renders the ImGui interface
  void render();

  // Generate random orders to simulate a lively market
  void simulate_market_activity();

private:
  MatchingEngine engine_;
  std::deque<ExecutionReport> trade_history_;
  OrderId next_order_id_ = 1;

  // Helper to render the actual lines in the order book
  void render_bids();
  void render_asks();

  // For testing/visuals, we need a way to peek into the order book.
  // We will expose a small snapshot mechanism or iterate over a custom getter.
  // For this simple portfolio demo, we'll let the app render a synthetic view
  // or we'll modify OrderBook to expose iterators.
};

} // namespace exchange
