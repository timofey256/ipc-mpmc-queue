#include <hiredis/hiredis.h>
#include <iostream>
#include <chrono>
#include <numeric>
#include <vector>
#include <algorithm>

uint64_t now_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

int main() {
    redisContext* c = redisConnect("127.0.0.1", 6379);
    if (c == nullptr || c->err) {
        std::cerr << "Connection error: " << c->errstr << "\n";
        return 1;
    }

    redisReply* reply = (redisReply*)redisCommand(c, "SUBSCRIBE test");
    freeReplyObject(reply);

    int count = 0;
    int target = 1000000;
    std::vector<uint64_t> latencies;

    while (count < target) {
        void* r = nullptr;
        if (redisGetReply(c, &r) == REDIS_OK && r != nullptr) {
            redisReply* msg = (redisReply*)r;
            if (msg->type == REDIS_REPLY_ARRAY && msg->elements == 3) {
                uint64_t sent_us = std::stoull(msg->element[2]->str);
                uint64_t recv_us = now_us();
                latencies.push_back(recv_us - sent_us);
                ++count;
            }
            freeReplyObject(msg);
        }
    }

    std::sort(latencies.begin(), latencies.end());
    double avg = std::accumulate(latencies.begin(), latencies.end(), 0ULL) / (double)latencies.size();

    auto percentile = [&](double p) {
        size_t idx = static_cast<size_t>(p * latencies.size());
        return latencies[std::min(idx, latencies.size() - 1)];
    };

    std::cout << "Received " << count << " messages\n";
    std::cout << "Latency (µs): avg=" << avg
              << ", p50=" << percentile(0.50)
              << ", p90=" << percentile(0.90)
              << ", p99=" << percentile(0.99)
              << ", max=" << latencies.back() << "\n";

    redisFree(c);
    return 0;
}

