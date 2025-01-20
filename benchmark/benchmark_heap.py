class FibNums:
    def __init__(self, fibn_minus_2, fibn_minus_1, fibn):
        self.fibn_minus_2  = fibn_minus_2
        self.fibn_minus_1 = fibn_minus_1
        self.fibn = fibn

def fib(n):
    fns = FibNums(0, 1, 1);
    if n == 0:
        return fns.fibn_minus_2
    elif n == 1:
        return fns.fibn_minus_1

    for i in range(2, n + 1):
        fns.fibn = fns.fibn_minus_1 + fns.fibn_minus_2
        fns.fibn_minus_2 = fns.fibn_minus_1
        fns.fibn_minus_1 = fns.fibn

    return fns.fibn

def main():
    num_iters = 1000000
    x = 2
    for i in range(0, num_iters):
        x = fib(90) - x // 2
    return x

y = main()
print(y)
