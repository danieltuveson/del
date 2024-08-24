# CC=clang
# CFLAGS = -O2 -g -Wall -Wextra -fsanitize=address

# CC=gcc
CFLAGS = -O2 -g -Wall -Wextra -DDEBUG=1
objects = common.o allocator.o lexer.o readfile.o linkedlist.o ast.o printers.o parser.o \
		  typecheck.o functiontable.o compiler.o vm.o error.o 
main = main.o
tests = tests.o

del: $(objects) $(main) 
	cc $(CFLAGS) -o del $(main) $(objects)

test: $(objects) $(tests)
	cc $(CFLAGS) -o test $(objects) $(tests)

clean:
	rm -f del test *.o
	rm -rf *.dSYM
