# # CFLAGS = -g -Wall -Wextra -Wno-padded -Wno-poison-system-directories -Wno-void-pointer-to-enum-cast -Wno-int-to-void-pointer-cast
CC=gcc
CFLAGS = -g -Wall -Wextra -Wno-return-type
objects = common.o allocator.o lexer.o readfile.o linkedlist.o ast.o printers.o parser.o typecheck.o \
          functiontable.o compiler.o vm.o
# compiler.o functiontable.o parser.o printers.o vm.o typecheck.o parser.tab.o lex.yy.o 
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
