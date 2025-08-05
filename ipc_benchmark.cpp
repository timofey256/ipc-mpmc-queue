#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <cassert>
#include "ipc_mpmc.h"

constexpr size_t queue_size = 1024;             // Must match default_queue_size in header
constexpr size_t n_messages = 100000000;          // Total messages to send
constexpr int n_producers = 2;
constexpr int n_consumers = 2;

struct complex_message_t {
    int  message_type;
    bool flag;
    char content[240];
};

std::atomic<size_t> messages_received(0);

void producer(size_t id, size_t count) {
    shm_mpmc_bounded_queue<complex_message_t> q("/mpmc_demo_queue", queue_size);
    for (size_t i = 0; i < count; ++i) {
        complex_message_t msg;
        msg.message_type = 1;
        msg.flag = false;
        std::snprintf(msg.content, sizeof(msg.content), "producer id: %zu | msg-%zu", id, i);   // fill payload

        while (!q.enqueue(msg)) {
            std::this_thread::yield();
        }
    }
}

void consumer() {
    shm_mpmc_bounded_queue<complex_message_t> q("/mpmc_demo_queue", queue_size);
    complex_message_t out;
    while (true) {
        if (q.dequeue(out)) {
            size_t current = messages_received.fetch_add(1, std::memory_order_relaxed);
            if (current + 1 >= n_messages) break;
        } else {
            if (messages_received.load(std::memory_order_relaxed) >= n_messages)
                break;  // Exit if all messages are already received by others
            std::this_thread::yield();
        }
    }
}

int main() {
    shm_unlink("/mpmc_demo_queue");
    shm_mpmc_bounded_queue<complex_message_t> init("/mpmc_demo_queue", queue_size);

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> producers, consumers;
    for (int i = 0; i < n_producers; ++i)
        producers.emplace_back(producer, i, n_messages / n_producers);
    for (int i = 0; i < n_consumers; ++i)
        consumers.emplace_back(consumer);

    for (auto& p : producers) p.join();
    for (auto& c : consumers) c.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    double latency = elapsed.count() * 1e6 / n_messages; // microseconds per message
    double throughput = n_messages / elapsed.count();    // messages per second

    std::cout << "Total messages: " << n_messages << "\n";
    std::cout << "Time elapsed:   " << elapsed.count() << " sec\n";
    std::cout << "Throughput:     " << throughput << " msgs/sec\n";
    std::cout << "Latency:        " << latency << " us/msg\n";

    return 0;
}
