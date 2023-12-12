# CFLAGS = -g -Wall -Wextra -Wno-padded -Wno-poison-system-directories -Wno-void-pointer-to-enum-cast -Wno-int-to-void-pointer-cast
CFLAGS = -g -Wall -Wextra -Wno-poison-system-directories -Wno-void-pointer-to-enum-cast -Wno-int-to-void-pointer-cast
objects = main.o compiler.o parser.o printers.o vm.o

lang : $(objects)
	cc $(CFLAGS) -o lang $(objects)

clean :
	rm lang $(objects)

