function fib(n) {
    let fibn_minus_2 = 0;
    let fibn_minus_1 = 1;
    let fibn = fibn_minus_1 + fibn_minus_2;
    if (n == 0) {
        return fibn_minus_2;
    } else if (n == 1) {
        return fibn_minus_1;
    }
    for (let i = 2; i <= n; i++) {
        fibn = fibn_minus_1 + fibn_minus_2;
        fibn_minus_2 = fibn_minus_1;
        fibn_minus_1 = fibn;
    }
    return fibn;
}

function main() {
    let num_iters = 1000000;
    let x = 2;
    for (let j = 0; j < num_iters; j++) {
        x = fib(90) - x / 2;
    }
    console.log(x);
}

main()
