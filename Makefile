# # CFLAGS = -g -Wall -Wextra -Wno-padded -Wno-poison-system-directories -Wno-void-pointer-to-enum-cast -Wno-int-to-void-pointer-cast
# CFLAGS = -g -Wall -Wextra -Wno-poison-system-directories -Wno-void-pointer-to-enum-cast -Wno-int-to-void-pointer-cast -Wvoid-pointer-to-int-cast 
# objects = main.o compiler.o parser.o printers.o vm.o
# 
# vb.tab.c vb.tab.h:	vb.y
# 	bison -t -v -d vb.y
# 
# lex.yy.c: vb.l vb.tab.h
# 	flex vb.l
# 
# lang : $(objects)
# 	cc $(CFLAGS) -o lang vb.tab.c lex.yy.c $(objects)
# 
# clean :
# 	rm lang $(objects) *.yy.c *.tab.c *.output
# 



