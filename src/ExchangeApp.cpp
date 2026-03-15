#include "ExchangeApp.h"
#include <imgui.h>
#include <random>

namespace exchange {

ExchangeApp::ExchangeApp() {
  simulate_market_activity();
}

void ExchangeApp::simulate_market_activity() {
  static std::mt19937 gen(1337);
  static std::uniform_int_distribution<Price> price_dist(90, 110);
  static std::uniform_int_distribution<Quantity> qty_dist(10, 500);
  static std::uniform_int_distribution<int> side_dist(0, 1);
  static std::uniform_int_distribution<int> iceberg_dist(0, 9); // 10% chance of iceberg

  for (int i = 0; i < 10; ++i) {
    Side s = (side_dist(gen) == 0) ? Side::Buy : Side::Sell;
    Quantity qty = qty_dist(gen);
    
    // Generate an iceberg order occasionally
    if (iceberg_dist(gen) == 0 && qty > 50) {
        Quantity visible_qty = qty / 5; // Show 20%
        engine_.add_limit_order(next_order_id_++, s, price_dist(gen), visible_qty, qty);
    } else {
        engine_.add_limit_order(next_order_id_++, s, price_dist(gen), qty);
    }
  }
}

void ExchangeApp::render() {
  simulate_market_activity();

  ImGui::SetNextWindowSize(ImVec2(1200, 600), ImGuiCond_FirstUseEver); // wider to fit both
  ImGui::Begin("High Performance Mock Exchange (C++20)", nullptr,
               ImGuiWindowFlags_NoCollapse);

  ImGui::Text("Order Book Render - Nanosecond Latency Backend");
  ImGui::Separator();

  // Two major columns: Left = Order Book, Right = Market Data Execution Feed
  if (ImGui::BeginTable("MainLayout", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
      ImGui::TableSetupColumn("OrderBook", ImGuiTableColumnFlags_WidthStretch, 0.6f);
      ImGui::TableSetupColumn("MarketFeed", ImGuiTableColumnFlags_WidthStretch, 0.4f);
      ImGui::TableNextRow();

      // LEFT COLUMN: ORDER BOOK
      ImGui::TableSetColumnIndex(0);
      if (ImGui::BeginTable("OrderBookTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("BIDS (Buyers)", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("ASKS (Sellers)", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        render_bids();

        ImGui::TableSetColumnIndex(1);
        render_asks();

        ImGui::EndTable();
      }

      // RIGHT COLUMN: MARKET FEED
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("Lock-Free SPSC Execution Feed");
      ImGui::Separator();
      
      // Consume RingBuffer and store historically for GUI
      auto& feed = engine_.get_market_data_feed();
      ExecutionReport report;
      while (feed.pop(report)) {
          // Keep last 50 reports
          if (trade_history_.size() >= 50) {
              trade_history_.pop_front();
          }
          trade_history_.push_back(report);
      }

      ImGui::BeginChild("ScrollingFeed", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
      for (const auto& r : trade_history_) {
          if (r.quantity == 0) {
              ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "CANCEL: Order %llu", r.id);
          } else {
              const char* side_str = (r.side == Side::Buy) ? "BUY" : "SELL";
              ImVec4 color = (r.side == Side::Buy) ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
              ImGui::TextColored(color, "FILL: %s %llu @ Px:%llu (ID:%llu)", side_str, r.quantity, r.price, r.id);
          }
      }
      
      // Auto-scroll to bottom
      if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
          ImGui::SetScrollHereY(1.0f);
          
      ImGui::EndChild();

      ImGui::EndTable();
  }

  ImGui::End();
}

void ExchangeApp::render_bids() {
  const auto &order_book = engine_.get_order_book();
  auto bids = order_book.get_top_bids(20);

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f)); // Green

  for (const auto &[price, level] : bids) {
    ImGui::Text("Px: %llu | Vol: %llu", price, level->total_volume);

    Order *curr = level->head;
    while (curr) {
      if (curr->total_remaining_qty > curr->quantity) {
          ImGui::BulletText("id:%llu (qty:%llu) [Iceberg Hidden:%llu]", curr->id, curr->quantity, curr->total_remaining_qty - curr->quantity);
      } else {
          ImGui::BulletText("id:%llu (qty:%llu)", curr->id, curr->quantity);
      }
      curr = curr->next;
    }
  }

  ImGui::PopStyleColor();
}

void ExchangeApp::render_asks() {
  const auto &order_book = engine_.get_order_book();
  auto asks = order_book.get_top_asks(20);

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f)); // Red

  for (const auto &[price, level] : asks) {
    ImGui::Text("Px: %llu | Vol: %llu", price, level->total_volume);

    Order *curr = level->head;
    while (curr) {
      if (curr->total_remaining_qty > curr->quantity) {
          ImGui::BulletText("id:%llu (qty:%llu) [Iceberg Hidden:%llu]", curr->id, curr->quantity, curr->total_remaining_qty - curr->quantity);
      } else {
          ImGui::BulletText("id:%llu (qty:%llu)", curr->id, curr->quantity);
      }
      curr = curr->next;
    }
  }

  ImGui::PopStyleColor();
}

} // namespace exchange
