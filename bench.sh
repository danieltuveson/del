make clean
make CFLAGS='-O2'
sudo chrt -f 99 perf stat -ddd ./del benchmark/benchmark.del
sudo chrt -f 99 perf stat -ddd python3 benchmark/benchmark.py
sudo chrt -f 99 perf stat -ddd lua benchmark/benchmark.lua