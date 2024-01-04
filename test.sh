flex test.l
bison -dv --verbose test.y
cc -o test test.tab.c lex.yy.c
