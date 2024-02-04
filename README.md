# dan's experimental language (del)
This is probably the last time I will try to make a programming language

`del` is a minimal programming language designed to be embedded in C applications. It is statically typed and has functions, classes, and the usual control flow mechanisms. It's main design goals are to be fast, simple, and intuitive. I intend to use it as a scripting language for writing an interactive fiction game, but we'll see if that ever happens.

Right now the syntax is kind of like Visual Basic, but I am probably going to rewrite the parser to use something closer to TypeScript's syntax.

The language is **not done**. It will hopefully be done in a few months.

## Things I've done right

### VM implementation
The VM is less than 300 lines of code (as of writing this). Everything in there does something that's absolutely necessary and uses a memory / speed efficient implmentation to do so. Calculations are done on the stack, local variables are stored in locals, and longer duration variables are stored in the heap. Most of the code is just a loop and a big switch statement on the opcodes. Accesses to the stack, heap, and locals only will involve indexing into their respective arrays and bumping a few counters. It's still not completely finished, but I think the overall design is really solid.

## A brief summary of my crimes

### Overuse of linked lists
I'm using linked lists basically everywhere in the parser, even though an array based list would likely be faster. My (maybe bad) justification for this is that at some point I want to switch the parser to use an bump allocator, and the reallocation that happens in an array list would gobble up memory. I also probably could be using a singlely linked list that just stores its head to avoid doing the weird "reset head" thing. But it works, so I'm leaving it for now.

### Global variable for the AST
I don't really like that I'm using a global variable for the AST, but I couldn't figure out how to return something from yyparse in bison. I have spent far more of my life trying to learn flex / bison than I would like, so until I decide to waste more of my time reading the terrible bison documentation, I'm leaving it this way. I hate bison, but I have realized that I hate hand-writing a parser much more.

I *probably* won't ever want more than one parser / AST to exist at a time even if I want more than one VM, since in theory we could reuse the global AST when we're done reading code into instructions for a given VM. But still, it makes things ugly, so I may change it.

### Lousy error messages
The error messages the bison parser gives by default are bad. But again, I really don't feel like learning about making them better right now.

### Awful mess of boilerplate for AST nodes
I feel like there should be a more elegant way of representing the AST than the mess of tagged unions that I have right now. But... it seems like this is the best way to do it! The AST code really is the ugliest part of the codebase.

### Not freeing memory...
I intend to move all of the AST / compiler code to use a big bump allocator. The only allocations needed for the VM are for Heap and maybe for Locals / Stack. Locals and Stack probably will never be allowed to grow too large, so it might be okay to stack allocate them. I'll also need to copy the debug symbols. 

### Not typechecking code...
Currently there is no typechecker. This is obviously something I need to do.

### Not freeing memory (again)...
I need to add a garbage collector to the VM. I don't think this will be that hard. Each value in the heap is prefixed by its length in uint64s. Since there should never realistically be more than 2^32 objects in the heap, I'm going to use the first bit of the length value to "mark" each object during mark and sweep garbage collection. That's my plan, anyway.

