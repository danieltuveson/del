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
}
```

## In progress / next item to do
- Implement garbage collection


## TODO
- Make typechecker validate that functions return values for all paths
- Remove "default" options from switch statements and search for missing cases.
- The `push_heap` function in the VM is basically just a consturctor. `push_heap` should just allocate and return a pointer to the memory, any constructor logic should be determined before runtime.
- Implement:
  - break / continue (how did I forget about that for so long?)
  - Sum types
  - Basic FFI, or at least add some simple IO
  - Arrays
  - Java-style interfaces
  - Function pointers
  - Module / namespace solutions
  - Error handling
- Windows support


## Things to continuously improve
- Make build less messy
- Improve error messages in lex / parse / typechecking stages
- Tests...
- Documentation / examples
- Improve performance (might require redesigning the VM or adding JIT for further improvements)

