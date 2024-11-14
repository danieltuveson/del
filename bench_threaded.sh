rm -f del_threaded
make clean
make CFLAGS='-O2 -DDEBUG=0 -DTHREADED_CODE_ENABLED=1'
cp del del_threaded

make clean
make CFLAGS='-O2 -DDEBUG=0 -DTHREADED_CODE_ENABLED=0'

sudo chrt -f 99 perf stat -ddd ./del_threaded benchmark/benchmark.del > /dev/null
sudo chrt -f 99 perf stat -ddd ./del benchmark/benchmark.del > /dev/null
sudo chrt -f 99 perf stat -ddd ./del_threaded benchmark/benchmark.del > /dev/null
sudo chrt -f 99 perf stat -ddd ./del benchmark/benchmark.del > /dev/null
sudo chrt -f 99 perf stat -ddd ./del_threaded benchmark/benchmark.del > /dev/null
sudo chrt -f 99 perf stat -ddd ./del benchmark/benchmark.del > /dev/null
sudo chrt -f 99 perf stat -ddd ./del_threaded benchmark/benchmark.del > /dev/null
sudo chrt -f 99 perf stat -ddd ./del benchmark/benchmark.del > /dev/null
rm -f del_threaded
