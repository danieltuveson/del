# # CFLAGS = -g -Wall -Wextra -Wno-padded -Wno-poison-system-directories -Wno-void-pointer-to-enum-cast -Wno-int-to-void-pointer-cast
CFLAGS = -g -Wall -Wextra -Wno-poison-system-directories -Wno-void-pointer-to-enum-cast -Wno-int-to-void-pointer-cast -Wvoid-pointer-to-int-cast 
objects = main.o compiler.o parser.o printers.o vm.o ast.o parser.tab.o lex.yy.o

lang: $(objects)
	cc $(CFLAGS) -o lang $(objects)
 
lex.yy.c lex.yy.h: lexer.l ast.c ast.h
	flex --header-file=lex.yy.h lexer.l

parser.tab.c parser.tab.h: parser.y lex.yy.c lex.yy.h
	bison -dv parser.y

parser.o: parser.tab.c parser.tab.h lex.yy.c lex.yy.h parser.h
	cc $(CFLAGS) -c parser.c

parser.tab.o: parser.tab.c parser.tab.h
	cc -c -o parser.tab.o parser.tab.c

lex.yy.o: lex.yy.c lex.yy.h
	cc -c -o lex.yy.o lex.yy.c

clean:
	rm -f lang *.o *.yy.h *.tab.h *.yy.c *.tab.c *.output
	rm -rf *.dSYM

