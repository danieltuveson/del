rm -f del_expect
make clean
make CFLAGS='-O2 -DDEBUG=0 -DTHREADED_CODE_ENABLED=1 -DEXPECT_ENABLED=1'

cp del del_expect

make clean
make CFLAGS='-O2 -DDEBUG=0 -DTHREADED_CODE_ENABLED=1 -DEXPECT_ENABLED=0'

sudo chrt -f 99 perf stat -ddd ./del_expect benchmark/benchmark.del 
sudo chrt -f 99 perf stat -ddd ./del benchmark/benchmark.del 
sudo chrt -f 99 perf stat -ddd ./del_expect benchmark/benchmark.del 
sudo chrt -f 99 perf stat -ddd ./del benchmark/benchmark.del 
sudo chrt -f 99 perf stat -ddd ./del_expect benchmark/benchmark.del 
sudo chrt -f 99 perf stat -ddd ./del benchmark/benchmark.del 
sudo chrt -f 99 perf stat -ddd ./del_expect benchmark/benchmark.del 
sudo chrt -f 99 perf stat -ddd ./del benchmark/benchmark.del 
rm -f del_expect
