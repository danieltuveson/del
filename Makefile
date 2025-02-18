# CFLAGS = -O2 -g -Wall -Wextra -fsanitize=address
# CC=clang
# CC=gcc
# CC=tcc

# Threading works with GCC, clang, and (surprisingly) tcc
# For C compilers that do not support threading, set -DTHREADED_CODE_ENABLED=0
# Debug flags:
# - For specific features: DEBUG_TEXT, DEBUG_LEXER, DEBUG_PARSER, DEBUG_TYPECHECKER,
#                          DEBUG_COMPILER, DEBUG_RUNTIME
# - For one-off things that I don't want to run in "prod" and will probably delete: DEBUG
# - To enable everything: DEBUG_ALL
CFLAGS = -O2 -g -Wall -Wextra -DGCOFF=0 -DTHREADED_CODE_ENABLED=1
# CFLAGS = -O2 -g -Wall -Wextra -DGCOFF=0 -DTHREADED_CODE_ENABLED=1 \
# 		 -DDEBUG_TEXT=1 -DDEBUG_COMPILER=1 -DDEBUG_RUNTIME=0
objects = common.o allocator.o linkedlist.o vector.o readfile.o lexer.o error.o \
	      parser.o ast.o functiontable.o typecheck.o compiler.o vm.o gc.o \
		  printers.o del.o

main = main.o
tests = tests.o

del: thread.h $(objects) $(main) 
	ar rc libdel.a $(objects)
	cc $(CFLAGS) -o del $(main) $(objects)

thread.h: bytecode.h vm.c
	bash threading.sh

# test: $(objects) $(tests)
# 	cc $(CFLAGS) -o test $(objects) $(tests)

install:
	@sudo cp del /usr/local/bin && echo "del installed at /usr/local/bin"
	@sudo cp libdel.a /usr/local/lib && echo "libdel.a installed at /usr/local/lib"
	@sudo cp del.h /usr/local/include && echo "del.h installed at /usr/local/include"

clean:
	rm -f generated_labels.h
	rm -f del *.o *.a
	rm -rf *.dSYM
