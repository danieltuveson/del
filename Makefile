CC=gcc
# CFLAGS = -O0 -g -Wall -Wextra -Wno-return-type
CFLAGS = -O2 -g -Wall -Wextra
objects = common.o allocator.o lexer.o readfile.o linkedlist.o ast.o printers.o parser.o typecheck.o \
          functiontable.o compiler.o vm.o
main = main.o
tests = tests.o
new_parser = new_parser.o

del: $(objects) $(main) 
	cc $(CFLAGS) -o del $(main) $(objects)

test: $(objects) $(tests)
	cc $(CFLAGS) -o test $(objects) $(tests)

clean:
	rm -f del test *.o
	rm -rf *.dSYM
