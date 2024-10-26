function runtests {
    ./del test/gc.del
    echo $?
}

make clean
make && runtests

