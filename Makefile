# # CFLAGS = -g -Wall -Wextra -Wno-padded -Wno-poison-system-directories -Wno-void-pointer-to-enum-cast -Wno-int-to-void-pointer-cast
cc=clang
CFLAGS = -g -Wall -Wextra
objects = main.o common.o compiler.o functiontable.o parser.o printers.o vm.o ast.o typecheck.o parser.tab.o lex.yy.o 

del: $(objects)
	cc $(CFLAGS) -o del $(objects)

lex.yy.c lex.yy.h: lexer.l ast.c ast.h
	flex --header-file=lex.yy.h lexer.l

parser.tab.c parser.tab.h: parser.y lex.yy.c lex.yy.h
	bison -dv parser.y

parser.o: parser.tab.c parser.tab.h lex.yy.c lex.yy.h parser.h
	cc $(CFLAGS) -c parser.c

parser.tab.o: parser.tab.c parser.tab.h
	cc $(CFLAGS) -c -o parser.tab.o parser.tab.c

lex.yy.o: lex.yy.c lex.yy.h
	cc -g -c -o lex.yy.o lex.yy.c

clean:
	rm -f del test *.o *.yy.h *.tab.h *.yy.c *.tab.c *.output
	rm -rf *.dSYM

