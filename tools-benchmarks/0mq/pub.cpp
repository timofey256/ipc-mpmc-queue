#include <zmq.hpp>
#include <chrono>
#include <thread>
#include <iostream>

uint64_t now_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

int main() {
    zmq::context_t ctx(1);
    zmq::socket_t pub(ctx, ZMQ_PUB);
    pub.bind("tcp://127.0.0.1:5555");

    // Give subscriber time to connect
    std::this_thread::sleep_for(std::chrono::seconds(10));

    constexpr int N = 10'000'0000;

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        uint64_t ts = now_us();
        pub.send(zmq::buffer(&ts, sizeof(ts)), zmq::send_flags::none);
    }
    auto end = std::chrono::steady_clock::now();

    double duration_sec = std::chrono::duration<double>(end - start).count();
    std::cout << "Publisher: Sent " << N << " messages in "
              << duration_sec << " sec (" << (N / duration_sec) << " msg/sec)\n";
}

