These are source codes for benchmarking pub/sub mechanisms in Redis and 0MQ.

### ZeroMQ
Build binaries.
```
cd ~/develop/
git clone git@github.com:zeromq/libzmq.git
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make -j$(nproc)
make install
```

Now, you will also need C++ bindings:
```sh
git clone https://github.com/zeromq/cppzmq.git ~/develop/cppzmq

export LD_LIBRARY_PATH=$HOME/.local/lib64:$LD_LIBRARY_PATH

# You can copy the source code for ZeroMQ now and compile it.
g++ pub.cpp -I ~/.local/include -I ~/develop/cppzmq -L ~/.local/lib64 -lzmq -O3 -o zmq_pub
g++ sub.cpp -I ~/.local/include -I ~/develop/cppzmq -L ~/.local/lib64 -lzmq -O3 -o zmq_sub
```

### Redis setup
Download the Redis package from your package manager.

Install the C++ bindings:
```
git clone https://github.com/redis/hiredis.git
cd hiredis/
make
sudo make install
```

Then, start the server.
```sh
redis-server --save '' --appendonly no --loglevel warning --hz 10

# Or if you want, consider running it on a single CPU for a better cache affinity:

taskset -c 0 redis-server --save '' --appendonly no --loglevel warning --hz 10
```

Then, compile the code:
```
g++ pub.cpp -lhiredis -O2 -o pub
g++ sub.cpp -lhiredis -O2 -o sub
```
