# dan's experimental language (del)
This is probably the last time I will try to make a programming language

`del` is a minimal programming language designed to be embedded in C applications. It is statically typed and has functions, classes, and the usual control flow mechanisms. It's main design goals are to be fast, simple, and intuitive. I intend to use it as a scripting language for writing an interactive fiction game, but we'll see if that ever happens.

The language is **not done**. It will hopefully be done in a few months.

## Building
Requires GNU Make and a C compiler (as of writing this, it will compile with GCC, Clang, and TCC). I've only tested the most recent versions on Linux, but I assume it would work on Mac as well. `del` comes pre-installed on Windows ;)
`make`
`./del examples/example.del`

## Example
``` js
class Range {
    start: int;
    end: int;
}

function sum(r : Range) : int {
    let sum = 0;
    for (let i = r.start; i <= r.end; i = i + 1) {
        sum = sum + i;
    }
    return sum;
}

function main() {
    let range = new Range(0, 100);
    let sum = sum(range);
}
```

## TODO
- Clean up parser
- Remove "default" options from switch statements and search for missing cases.
- Fix string implementation
- Finish implementing classes 
- Fix scope issues
- Implement basic FFI, or at least add some simple IO
- Implement arrays
- Implement interfaces
- Implement garbage collection
- Improve performance (maybe require redesigning the VM)