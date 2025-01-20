
fibHelper n fibnMinus2 fibnMinus1
    | n == 0 = fibnMinus2
    | otherwise = fibHelper (n - 1) fibnMinus1 (fibnMinus2 + fibnMinus1)

fib n = fibHelper n 0 1

loop :: Int -> Int -> Int
loop n acc =
    if n == 0 then
        acc
    else
        loop (n - 1) ((fib 90) - acc `div` 2)

main :: IO ()
main = do 
    let num_iters = 1000000
    let x = 2
    print (loop num_iters 0)
