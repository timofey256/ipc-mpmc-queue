#include <zmq.hpp>
#include <thread>
#include <chrono>
#include <iostream>
#include <cstring>

uint64_t now_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

int main() {
    zmq::context_t ctx(1);
    zmq::socket_t sub(ctx, ZMQ_SUB);
    sub.connect("tcp://127.0.0.1:5555");
    sub.set(zmq::sockopt::subscribe, "");

    // Give ZeroMQ time to establish subscription internally
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    constexpr int N = 10'000'00;
    uint64_t total_latency = 0;
    uint64_t max_latency = 0;

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        zmq::message_t msg;
        sub.recv(msg, zmq::recv_flags::none);

        if (msg.size() != sizeof(uint64_t)) {
            std::cerr << "Invalid message size: " << msg.size() << "\n";
            continue;
        }

        uint64_t sent;
        std::memcpy(&sent, msg.data(), sizeof(sent));
        uint64_t latency = now_us() - sent;
        total_latency += latency;
        if (latency > max_latency) max_latency = latency;
    }
    auto end = std::chrono::steady_clock::now();

    double duration_sec = std::chrono::duration<double>(end - start).count();
    double avg_latency = total_latency / static_cast<double>(N);

    std::cout << "Subscriber: Received " << N << " messages in "
              << duration_sec << " sec (" << (N / duration_sec) << " msg/sec)\n";
    std::cout << "Average latency: " << avg_latency << " µs\n";
    std::cout << "Max latency: " << max_latency << " µs\n";
}

