function fib(n)
    local fns = {
        fibn_minus_2 = 0,
        fibn_minus_1 = 1,
        fibn = 1
    }
    if n == 0 then
        return fns.fibn_minus_2
    elseif n == 1 then
        return fns.fibn_minus_1
    end

    for i = 2, n do
        fns.fibn = fns.fibn_minus_1 + fns.fibn_minus_2
        fns.fibn_minus_2 = fns.fibn_minus_1
        fns.fibn_minus_1 = fns.fibn
    end

    return fns.fibn
end

function main()
    local num_iters = 1000000
    local x = 2
    for i = 0, num_iters do
        x = fib(90) - x / 2
    end
    return x
end

y = main()
print(y)
