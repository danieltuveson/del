def fib(n):
    fibn_minus_2 = 0
    fibn_minus_1 = 1
    fibn = fibn_minus_1 + fibn_minus_2
    if n == 0:
        return fibn_minus_2
    elif n == 1:
        return fibn_minus_1

    i = 2
    while i < n + 1:
        fibn = fibn_minus_1 + fibn_minus_2
        fibn_minus_2 = fibn_minus_1
        fibn_minus_1 = fibn
        i += 1

    return fibn

def main():
    num_iters = 1000000
    x = 2
    i = 0
    while i < num_iters:
        x = fib(90) - x // 2
        i += 1
    return x

y = main()
print(y)
