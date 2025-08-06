// Based on Dmitry Vyukov's MPMC Queue

#include <atomic>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <new>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>   
#include <unistd.h>  

namespace {

constexpr std::size_t default_queue_size = 1024; // must be a power of 2

template <typename Payload>
struct queue_data_t {
    std::atomic<std::size_t> enqueue_pos;
    std::atomic<std::size_t> dequeue_pos;
    std::size_t              mask;

    // tried to pad it in order to avoid false sharing but seems it has no effect, so i removed it
    struct cell {
        std::atomic<std::size_t> seq;
        Payload                  data;
    } buf[default_queue_size];
};

}

template <typename Payload>
inline void init_queue(queue_data_t<Payload>* q) {
    q->enqueue_pos.store(0, std::memory_order_relaxed);
    q->dequeue_pos.store(0, std::memory_order_relaxed);
    q->mask = default_queue_size - 1;

    for (std::size_t i = 0; i < default_queue_size; i++) {
        q->buf[i].seq.store(i, std::memory_order_relaxed);
    }
}

template<typename Payload>
class shm_mpmc_bounded_queue {
public:
    explicit shm_mpmc_bounded_queue(const std::string& shm_name, bool create_segment=true)
        : shm_name_(shm_name), fd_(-1), data_(nullptr), owner_(false)
    {
        open_segment(create_segment);
    }

    ~shm_mpmc_bounded_queue() {
        if (data_) munmap(data_, sizeof(queue_data_t<Payload>));
        if (fd_ >= 0) close(fd_);
        if (owner_) shm_unlink(shm_name_.c_str());
    }

    bool enqueue(const Payload& v) {
        auto* q   = data_;
        auto  pos = q->enqueue_pos.load(std::memory_order_relaxed);
        cell_t* c;

        for (;;) {
            c   = &q->buf[pos & q->mask];
            auto seq = c->seq.load(std::memory_order_acquire);
            auto dif = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos);

            if (dif == 0) {
                if (q->enqueue_pos.compare_exchange_weak(
                        pos, pos + 1, std::memory_order_relaxed))
                    break;
            } else if (dif < 0) {
                return false;                          // full
            } else {
                pos = q->enqueue_pos.load(std::memory_order_relaxed);
            }
        }
        c->data = v;
        c->seq.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool dequeue(Payload& out) {
        auto* q   = data_;
        auto  pos = q->dequeue_pos.load(std::memory_order_relaxed);
        cell_t* c;

        for (;;) {
            c   = &q->buf[pos & q->mask];
            auto seq = c->seq.load(std::memory_order_acquire);
            auto dif = static_cast<std::intptr_t>(seq) -
                       static_cast<std::intptr_t>(pos + 1);

            if (dif == 0) {
                if (q->dequeue_pos.compare_exchange_weak(
                        pos, pos + 1, std::memory_order_relaxed))
                    break;
            } else if (dif < 0) {
                return false;                          // empty
            } else {
                pos = q->dequeue_pos.load(std::memory_order_relaxed);
            }
        }
        out = c->data;
        c->seq.store(pos + q->mask + 1, std::memory_order_release);
        return true;
    }

private:
    using cell_t = typename queue_data_t<Payload>::cell;


    std::string             shm_name_;
    int                     fd_;
    queue_data_t<Payload>*  data_;
    bool                    owner_;

    shm_mpmc_bounded_queue(shm_mpmc_bounded_queue const&) = delete;
    void operator=(shm_mpmc_bounded_queue const&) = delete;

    void open_segment(bool create_or_attach) {
        int flags = O_RDWR | (create_or_attach ? O_CREAT : 0);
        fd_       = shm_open(shm_name_.c_str(), flags, 0666);

        if (fd_ < 0) throw std::runtime_error("shm_open failed");

        struct stat st {};
        fstat(fd_, &st);
        bool need_init = (st.st_size == 0);
        if (need_init) {
            if (ftruncate(fd_, sizeof(queue_data_t<Payload>)) != 0) throw std::runtime_error("ftruncate failed");
        }
        void* p = mmap(nullptr, sizeof(queue_data_t<Payload>), PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (p == MAP_FAILED) throw std::runtime_error("mmap failed");
        data_ = static_cast<queue_data_t<Payload>*>(p);
        if (need_init) {
            owner_ = true;
            init_queue<Payload>(data_);
        }
    }
};

