#pragma once

#include <cstdint>

namespace exchange {

using OrderId = uint64_t;
using Price = uint64_t; // Prices can be represented as integers (e.g.
                        // multiplied by 10000)
using Quantity = uint64_t;

enum class Side : uint8_t { Buy, Sell };

struct alignas(64) Order {
  OrderId id;
  Side side;
  Price price;
  Quantity quantity;             // Visible peak
  Quantity total_remaining_qty;  // Total hidden size
  Quantity display_qty;          // Maximum visible peak upon refill

  // Pointers for a doubly linked list in the order book's price level
  Order *prev = nullptr;
  Order *next = nullptr;

  Order() = default;
  Order(OrderId id, Side side, Price price, Quantity visible_qty, Quantity total_qty = 0)
      : id(id), side(side), price(price), quantity(visible_qty), 
        total_remaining_qty(total_qty == 0 ? visible_qty : total_qty), 
        display_qty(visible_qty), prev(nullptr), next(nullptr) {}
};

// Struct for piping data out of the engine without locking
struct ExecutionReport {
  OrderId id;
  Price price;
  Quantity quantity;
  Side side;
};

} // namespace exchange
