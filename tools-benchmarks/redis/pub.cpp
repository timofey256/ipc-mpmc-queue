#include <hiredis/hiredis.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <sstream>

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

    int count = 1000000;
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Let subscriber connect

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < count; ++i) {
        uint64_t timestamp = now_us();
        std::ostringstream oss;
        oss << timestamp;
        redisCommand(c, "PUBLISH test %s", oss.str().c_str());
    }
    auto end = std::chrono::high_resolution_clock::now();

    double elapsed = std::chrono::duration<double>(end - start).count();
    std::cout << "Published " << count << " messages in " << elapsed << " sec\n";
    std::cout << "Throughput: " << (count / elapsed) << " msg/sec\n";

    redisFree(c);
    return 0;
}

