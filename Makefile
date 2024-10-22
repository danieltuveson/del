# CFLAGS = -O2 -g -Wall -Wextra -fsanitize=address
# CC=clang
# CC=gcc
# CC=tcc
# CFLAGS = -O2 -g -Wall -Wextra -DDEBUG=1 -DTHREADED_CODE_ENABLED=1

# Threading option exists, but disabled by default since I didn't see a consistent speedup
CFLAGS = -O2 -g -Wall -Wextra -DGCOFF=0 -DDEBUG=0 -DTHREADED_CODE_ENABLED=0
# CFLAGS = -O2 -g -Wall -Wextra -DGCOFF=0 -DDEBUG=1 -DTHREADED_CODE_ENABLED=0
objects = common.o allocator.o lexer.o readfile.o linkedlist.o ast.o printers.o parser.o \
		  typecheck.o functiontable.o compiler.o vm.o error.o 
main = main.o
tests = tests.o

del: $(objects) $(main) 
	cc $(CFLAGS) -o del $(main) $(objects)

test: $(objects) $(tests)
	cc $(CFLAGS) -o test $(objects) $(tests)

clean:
	rm -f del del_threaded test *.o
	rm -rf *.dSYM
