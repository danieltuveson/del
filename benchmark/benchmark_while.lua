function fib(n)
    local fbn
    local fibn_minus_2 = 0
    local fibn_minus_1 = 1
    local fibn = fibn_minus_1 + fibn_minus_2
    if n == 0 then
        return fibn_minus_2
    elseif n == 1 then
        return fibn_minus_1
    end

    local i = 2
    while i <= n do
        fibn = fibn_minus_1 + fibn_minus_2
        fibn_minus_2 = fibn_minus_1
        fibn_minus_1 = fibn
        i = i + 1
    end

    return fibn
end

function main()
    local num_iters = 1000000
    local x = 2
    local i = 0
    while i < num_iters do
        x = fib(90) - x / 2
        i = i + 1
    end
    return x
end

y = main()
print(y)
