# CFLAGS = -O2 -g -Wall -Wextra -fsanitize=address
# CC=clang
# CC=gcc
# CC=tcc

# Threading works with GCC, clang, and (surprisingly) tcc
# For C compilers that do not support threading, set -DTHREADED_CODE_ENABLED=0
CFLAGS = -O2 -g -Wall -Wextra -DGCOFF=0 -DDEBUG=0 -DTHREADED_CODE_ENABLED=1
# CFLAGS = -O2 -g -Wall -Wextra -DGCOFF=0 -DDEBUG=1 -DTHREADED_CODE_ENABLED=1
objects = common.o allocator.o linkedlist.o vector.o readfile.o lexer.o error.o \
	      parser.o ast.o functiontable.o typecheck.o compiler.o vm.o \
		  printers.o del.o

main = main.o
tests = tests.o

del: $(objects) $(main) 
	ar rc libdel.a $(objects)
	cc $(CFLAGS) -o del $(main) $(objects)

# test: $(objects) $(tests)
# 	cc $(CFLAGS) -o test $(objects) $(tests)

clean:
	rm -f del *.o *.a
	rm -rf *.dSYM
