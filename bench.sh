make clean
make 

sudo chrt -f 99 perf stat -ddd ./del benchmark/benchmark.del
sudo chrt -f 99 perf stat -ddd python3 benchmark/benchmark.py
sudo chrt -f 99 perf stat -ddd lua benchmark/benchmark.lua
# sudo chrt -f 99 perf stat -ddd ../quickjs-2024-01-13/qjs benchmark/benchmark.js
