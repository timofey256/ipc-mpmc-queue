# Inter-process Bounded MPMC Queue
Uses Linux shared memory ring buffers. 

This is a supplementary material to my [blog post](https://timofey256.github.io/blog/2025/pubsub-using-mpmc/).

---

### Project structure

- `tools-benchmarks/` folder contains my small pub/sub programs to test performance of Redis and ZeroMQ.

- `src/` folder contains the original implementatio of [Dmitry Vyukov's Bounded MPMC queue](https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue).

- root contains the final results with benchmarking code.

### Benchmarking results

```
Number of producers: 2
Number of consumers: 2
Total messages: 500000000
Time elapsed:   54.6424 sec
Throughput:     9.1504e+06 msgs/sec
Average time per logical message: 0.109285 us/msg

========

Number of producers: 4
Number of consumers: 4
Total messages: 500000000
Time elapsed:   67.9451 sec
Throughput:     7.35888e+06 msgs/sec
Average time per logical message: 0.13589 us/msg

=======

Number of producers: 8
Number of consumers: 8
Total messages: 500000000
Time elapsed:   76.3229 sec
Throughput:     6.55111e+06 msgs/sec
Average time per logical message: 0.152646 us/msg
```

Note that the throughput goes down because of a higher contention.
