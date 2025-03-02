# dan's experimental language (del)
This is probably the last time I will try to make a programming language

`del` is a minimal programming language designed to be embedded in C applications. It is statically typed and has functions, classes, and the usual control flow mechanisms. Its main design goals are to be fast, simple, and intuitive. I intend to use it as a scripting language for writing an interactive fiction game, but we'll see if that ever happens.

The language is **not done**. It will hopefully be done in a few months.

## Building
Requires GNU Make and a C compiler (as of writing this, it will compile with GCC, Clang, and TCC). I've only tested the most recent versions on Linux, but I assume it would work on Mac as well. `del` comes pre-installed on Windows ;)

``` bash
make
./del examples/example.del
```

If you want to be able to just run it as `del examples/examples.del`, then `make install` will add it to `/usr/local/bin` and will add `libdel.a` to `/usr/local/lib`. This may or may not be the place your system likes to have user-built libraries / executables.


## Example
``` js
class Range {
    start: int;
    end: int;
}

function sum(r : Range) : int {
    let sum = 0;
    for let i = r.start; i <= r.end; i++ {
        sum = sum + i;
    }
    return sum;
}

function main() {
    let range = new Range(0, 100);
    let sum = sum(range);
    println(sum);
}
```

## In progress / next item to do
- Implement garbage collection

## TODO
- Make sure there is no recursion in the compiler / runtime that could lead to a stackoverflow in the C code. Cases to watch out for:
  - GC can't recursively walk structures (otherwise marking a recursive data structure could potentially overflow the stack).
  - Lexer should look for / reject highly nested expressions and blocks of a certain depth by counting '{' and '(' (and maybe '<' for generics, though that may need to be done in the parser since '<' is also used for comparisons). This would prevent `if x > 1 { if x > 2 { ... if x > n { ... } ... } }` from blowing up
  - Probably more cases to watch out for, but those are the ones I can think of
- Remove "default" options from switch statements and search for missing cases.
- The `push_heap` function in the VM is basically just a consturctor. `push_heap` should just allocate and return a pointer to the memory, any constructor logic should be determined before runtime.
- Variables that could potentially be unset currently default to 0, but should give a compile error.
- Implement:
  - Array literals
  - Sum types
  - Basic FFI, or at least add some simple IO
  - Java-style interfaces
  - Function pointers
  - Module / namespace solutions
  - Error handling
- Windows support
- Low hanging fruit that I forgot about
  - ! (logical not)
  - Float stuff: ability to check for NaN, infinities, etc
  - Make increment and decrement work for properties again
  - Bitwise operators (maybe)
  - Read function (maybe)


## Things to continuously improve
- Make build less messy
- Improve error messages in lex / parse / typechecking stages
- Tests...
- Documentation / examples
- Improve performance (might require redesigning the VM or adding JIT for further improvements)

