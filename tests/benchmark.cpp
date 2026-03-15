#include "MatchingEngine.h"
#include <benchmark/benchmark.h>
#include <vector>
#include <algorithm>

using namespace exchange;

// Lambda for P99
auto P99 = [](const std::vector<double>& v) -> double {
  if (v.empty()) return 0;
  std::vector<double> copy = v;
  std::sort(copy.begin(), copy.end());
  size_t idx = static_cast<size_t>(copy.size() * 0.99);
  return copy[idx];
};

static void BM_AddLimitOrder_NoMatch(benchmark::State &state) {
  MatchingEngine engine;
  OrderId id = 1;
  for (auto _ : state) {
    state.PauseTiming();
    Price p = 100 + (id % 100);
    state.ResumeTiming();

    engine.add_limit_order(id++, Side::Buy, p, 10);
  }
}
BENCHMARK(BM_AddLimitOrder_NoMatch)->Repetitions(10)->ComputeStatistics("p99", P99)->DisplayAggregatesOnly(true);

static void BM_MatchLimitOrder(benchmark::State &state) {
  MatchingEngine engine;
  for (int i = 0; i < 10000; ++i) {
    engine.add_limit_order(i + 1, Side::Sell, 100 + (i % 100), 10);
  }

  OrderId taker_id = 100000;
  for (auto _ : state) {
    state.PauseTiming();
    state.ResumeTiming();

    engine.add_limit_order(taker_id++, Side::Buy, 100, 10);

    state.PauseTiming();
    engine.add_limit_order(taker_id++, Side::Sell, 100, 10);
    state.ResumeTiming();
  }
}
BENCHMARK(BM_MatchLimitOrder)->Repetitions(10)->ComputeStatistics("p99", P99)->DisplayAggregatesOnly(true);

static void BM_MarketOrderMatch(benchmark::State &state) {
  MatchingEngine engine;
  for (int i = 0; i < 10000; ++i) {
    engine.add_limit_order(i + 1, Side::Sell, 100 + (i % 10), 10);
  }

  OrderId taker_id = 100000;
  for (auto _ : state) {
    engine.add_market_order(taker_id++, Side::Buy, 10);

    state.PauseTiming();
    engine.add_limit_order(taker_id++, Side::Sell, 105, 10);
    state.ResumeTiming();
  }
}
BENCHMARK(BM_MarketOrderMatch)->Repetitions(10)->ComputeStatistics("p99", P99)->DisplayAggregatesOnly(true);

static void BM_OrderCancellation(benchmark::State &state) {
  MatchingEngine engine;
  OrderId id = 1;
  for (auto _ : state) {
    state.PauseTiming();
    engine.add_limit_order(id, Side::Buy, 100, 10);
    state.ResumeTiming();

    engine.cancel_order(id);

    state.PauseTiming();
    id++;
    state.ResumeTiming();
  }
}
BENCHMARK(BM_OrderCancellation)->Repetitions(10)->ComputeStatistics("p99", P99)->DisplayAggregatesOnly(true);

BENCHMARK_MAIN();
