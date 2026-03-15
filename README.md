<div align="center">
  
# 🚀 High-Performance C++ Mock Exchange
  
**A blazingly fast, single-threaded Limit Order Book (LOB) and Matching Engine written in modern C++20.**

[![C++20](https://img.shields.io/badge/C++-20-blue.svg?style=flat&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/compiler_support)
[![CMake](https://img.shields.io/badge/CMake-3.14+-green.svg?style=flat&logo=cmake)](https://cmake.org)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![GUI](https://img.shields.io/badge/GUI-Dear%20ImGui-blueviolet.svg)](https://github.com/ocornut/imgui)

</div>

---

This project demonstrates low-latency programming techniques suitable for quantitative development and high-frequency trading (HFT) environments. It includes a native **Dear ImGui** application that visually simulates order flow through the engine in real-time.

---

## ✨ Key Features & Architecture

*   ⚡ **Zero Dynamic Allocations in the Critical Path**: The engine uses a custom, statically pre-allocated `MemoryPool` that issues `Order` objects in $\mathcal{O}(1)$ time, entirely avoiding slow system calls like `new` or `malloc` when matching orders.
*   ⚡ **$\mathcal{O}(1)$ Price Level Operations**: The order book utilizes intrusive doubly-linked lists. Order objects hold their own `prev` and `next` pointers, meaning deletions/fills at the head of a price level (Price-Time Priority) happen in constant time.
*   ⚡ **$\mathcal{O}(1)$ Order Cancellations**: The exchange maintains a flat array map of Order IDs to memory locations. Canceled orders are snipped out of the intrusive linked lists instantly without searching through price structures.
*   🧊 **Iceberg Orders (Hidden Liquidity)**: Supports orders with hidden sizes. The engine seamlessly refills the visible "peak" $\mathcal{O}(1)$ when depleted, correctly re-inserting the tranche at the tail of the price level to yield time-priority as required by real exchange rules.
*   🔄 **Lock-Free Market Data Feed (SPSC Queue)**: Integrates a pure `std::atomic` Ring Buffer utilizing `memory_order_release` and `memory_order_acquire`. The engine pushes out `ExecutionReport`s (fills and cancels) to an external thread without ever grabbing a mutex, eliminating thread stalls in the critical path.
*   🏎️ **Sub-Microsecond Latency**: Limits orders match in **~1.1 microseconds** (P99) and market orders in **~0.6 microseconds** (P99), verified via Google Benchmark.
*   🧪 **Google Test Suite**: Dedicated unit tests verifying complex edge cases like partial-fills, overlapping limits, and Iceberg refill priority logic.
*   🖥️ **Native GUI**: A high-framerate renderer built with `Dear ImGui` and `GLFW` to visualize the engine's internal state, including hidden iceberg quantities and the live SPSC execution event feed.

---

## 🛠 Prerequisites

Ensure you have the following installed on your system:

*   **Compiler**: A C++20 capable compiler (`clang` or `gcc`)
*   **Build System**: `CMake` (Version 3.14+)
*   **Graphics**: `GLFW` (For the GUI. On macOS, install via `brew install glfw`)

---

## 💻 Building and Running

### 1️⃣ Build the GUI App
To run the live mock exchange visualizer:

```bash
cd mock_exchange
cmake -B build -S .
cmake --build build -j$(sysctl -n hw.ncpu)
./build/mock_exchange_gui
```

### 2️⃣ Run the Benchmark Suite
To measure pure nanosecond limit-order throughput (ensure you compile in `Release` mode!):

```bash
mkdir build-release
cd build-release
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(sysctl -n hw.ncpu)
./tests/benchmark_matching
```

### 3️⃣ Run the Unit Tests
To run the suite of functional tests and ensure accuracy:

```bash
./build/tests/test_matching
```

---

## ⚡ Performance & Benchmarks

The core objective of this mock exchange is **deterministic, ultra-low latency**. By avoiding dynamic memory allocation (`new`/`delete`) and pointer-chasing (using intrusive lists), the matching engine guarantees highly predictable execution times, even under heavy load.

The benchmarks are verified using the **Google Benchmark** suite, running on a single thread. The metrics represent the **P99 tail latency** for core exchange operations:

| Operation | Latency (P99) | Description |
| :--- | :--- | :--- |
| **Market Order Match** | `~600 ns` (0.6 µs) | Instantly sweeping through resting liquidity. |
| **Limit Order Add (No Match)** | `~570 ns` (0.57 µs) | Adding a new limit order to a deep price level queue. |
| **Limit Order (Match & Refill)** | `~1150 ns` (1.15 µs) | Matching against resting liquidity, emitting an `ExecutionReport` to the Ring Buffer, and refilling an Iceberg order. |

> **Note**: These tests demonstrate that complex market logic (like Iceberg hidden quantities and Lock-Free Ring Buffer dispatching) can be executed without stalling the hot path, preserving sub-microsecond speeds.

---

## 🧠 Memory Architecture Deep Dive

The standard, naive way to build a limit order book uses standard libraries: `std::map<Price, std::list<Order>>`. However, standard libraries inherently call `new` when inserting elements, destroying latency.

This engine bypasses this bottleneck by allocating a vast block of memory on startup:
1. `MemoryPool` reserves 1,000,000 blank orders in a single underlying `std::vector` (or equivalent structure).
2. As a client submits a new limit order, the engine queries the `MemoryPool` free-list index, grabs a memory address, and uses **placement-new** to construct the order *there*.
3. The order pointer is then pushed to the `OrderBook`'s intrusive list (`PriceLevel`). 

By enforcing that all active `Order`s are completely contiguous and pre-allocated, the engine eliminates context-switching latency during high market volatility.

---

<div align="center">
  <i>Developed by <a href="https://github.com/slimboi34">slimboi34</a>.</i>
</div>
