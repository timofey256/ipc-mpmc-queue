#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <cassert>
#include "mpmc.h" // Your Vyukov queue

constexpr size_t QUEUE_SIZE = 1024;
constexpr size_t N_MESSAGES = 100000000;
constexpr int N_PRODUCERS = 2;
constexpr int N_CONSUMERS = 2;

mpmc_bounded_queue<int> queue(QUEUE_SIZE);
std::atomic<size_t> messages_received(0);

void producer_thread(int id, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        while (!queue.enqueue(static_cast<int>(i))) {
            std::this_thread::yield();
        }
    }
}

void consumer_thread() {
    int value;
    while (true) {
        if (queue.dequeue(value)) {
            size_t current = messages_received.fetch_add(1, std::memory_order_relaxed);
            if (current + 1 >= N_MESSAGES) break;
        } else {
            if (messages_received.load(std::memory_order_relaxed) >= N_MESSAGES)
                break;  // Exit if all messages are already received by others
            std::this_thread::yield();
        }
    }
}

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> producers, consumers;

    for (int i = 0; i < N_PRODUCERS; ++i)
        producers.emplace_back(producer_thread, i, N_MESSAGES / N_PRODUCERS);
    for (int i = 0; i < N_CONSUMERS; ++i)
        consumers.emplace_back(consumer_thread);

    for (auto& p : producers) p.join();
    for (auto& c : consumers) c.join();

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    double total_time_sec = elapsed.count();
    double throughput = N_MESSAGES / elapsed.count();
    double latency_per_message_us = (total_time_sec * 1e6) / N_MESSAGES;  // in microseconds

    assert(messages_received.load() == N_MESSAGES);
    std::cout << "Total messages: " << N_MESSAGES << "\n";
    std::cout << "Time elapsed: " << elapsed.count() << " sec\n";
    std::cout << "Throughput: " << throughput << " messages/sec\n";
    std::cout << "Latency per message:   " << latency_per_message_us << " µs\n";

    return 0;
}
