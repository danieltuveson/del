function fib(n)
    local fibn_minus_2 = 0
    local fibn_minus_1 = 1
    local fibn = fibn_minus_1 + fibn_minus_2
    if n == 0 then
        return fibn_minus_2
    elseif n == 1 then
        return fibn_minus_1
    end

    for i = 2, n do
        fibn = fibn_minus_1 + fibn_minus_2
        fibn_minus_2 = fibn_minus_1
        fibn_minus_1 = fibn
    end

    return fibn
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
