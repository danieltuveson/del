# cc -c test.c
flex test.l
bison -dv --verbose test.y
cc -g -O0 -o test test.tab.c lex.yy.c
