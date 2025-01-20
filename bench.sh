make clean
make 

# Del
sudo chrt -f 99 perf stat -ddd ./del benchmark/benchmark.del

# sudo chrt -f 99 perf stat -ddd ./del benchmark/benchmark_heap.del
# sudo chrt -f 99 perf stat -ddd python3 benchmark/benchmark.py
# sudo chrt -f 99 perf stat -ddd python3 benchmark/benchmark_heap.py
# sudo chrt -f 99 perf stat -ddd lua benchmark/benchmark.lua
# sudo chrt -f 99 perf stat -ddd lua benchmark/benchmark_heap.lua
# sudo chrt -f 99 perf stat -ddd luajit -joff benchmark/benchmark.lua
# sudo chrt -f 99 perf stat -ddd luajit -joff benchmark/benchmark_heap.lua

# ghc benchmark/benchmark.hs
# sudo chrt -f 99 perf stat -ddd benchmark/benchmark
# sudo chrt -f 99 perf stat -ddd ../luau-0.654/luau benchmark/benchmark.lua
# sudo chrt -f 99 perf stat -ddd ../quickjs-2024-01-13/qjs benchmark/benchmark.js
